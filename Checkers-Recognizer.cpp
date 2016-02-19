#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include <algorithm>
using namespace cv;
using namespace std;

float odleglosc(Point2f pierwszy, Point2f drugi) {
	float x = 0.0, y = 0.0;
	if (pierwszy.x > drugi.x)
		x = pierwszy.x - drugi.x;
	else
		x = drugi.x - pierwszy.x;
	if (pierwszy.y > drugi.y)
		y = pierwszy.y - drugi.y;
	else 
		y = drugi.y - pierwszy.y;
	return sqrt(x*x + y*y);
}

float odejmij(float x, float y) {
	if (x > y)
		return x - y;
	else
		return y - x;
}

Point2f punkt_przeciecia(Point2f p1, Point2f p2, Point2f q1, Point2f q2) {
	float a1 = (p1.y - p2.y) / (p1.x - p2.x);
	float a2 = (q1.y - q2.y) / (q1.x - q2.x);
	float b1 = (p1.y - a1*p1.x);
	float b2 = (q1.y - a2*q1.x);
	return Point2f((b2 - b1) / (a1 - a2), a2*((b2 - b1) / (a1 - a2))+b2);
}

int main(int argc, char* argv[])
{
	// przechwytujemy obraz z kamery
	VideoCapture cap(0);
	if (!cap.isOpened()) {
		cout << "Cannot open the video cam" << endl;
		return -1;
	} cout << "Frame size : " << cap.get(CV_CAP_PROP_FRAME_WIDTH) << " x " << cap.get(CV_CAP_PROP_FRAME_HEIGHT) << endl;

	namedWindow("Regulacja", 1);
	int prog = 64;
	createTrackbar("Regulator", "Regulacja", &prog, 128);

	Point2f lewy_gora, lewy_dol, prawy_gora, prawy_dol, srodek, rogi[4];
	bool sprawdz_plansze = true;
	int counter = 0;

	while (1)
	{
		Mat frame, frame_copy, frame_rectangle, plansza(480, 440, CV_8UC3, Scalar::all(40));

		// tworzymy i zerujemy tablicê z ustawieniem pionków i zmienne ze stanem gry
		int gracz1 = 0, gracz2 = 0;
		short pole[8][8];
		for (int i = 0; i < 8; i++)
			for (int j = 0; j < 8; j++)
				pole[i][j] = 0;

		// odczytujemy pojedyncz¹ klatkê z obrazu z kamery
		if (!cap.read(frame)) {
			cout << "Cannot read a frame from video stream" << endl;
			break;
		}

		Mat threshold_output;
		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;
		frame_copy = frame.clone();

		// transformacje na obrazie do wykrywania krawêdzi planszy
		if (sprawdz_plansze) {

			lewy_gora = { 320.0, 240.0 }; 
			lewy_dol = { 320.0, 240.0 }; 
			prawy_gora = { 320.0, 240.0 };
			prawy_dol = { 320.0, 240.0 }; 
			srodek = { 320.0, 240.0 };

			for (int k = 0; k < 2; k++) {
				// szukaj bia³e pola
				if (k == 0) {
					cvtColor(frame, frame_rectangle, CV_BGR2GRAY);
					erode(frame_rectangle, frame_rectangle, Mat(), Point(-1, -1), 2, 1, 1);
					GaussianBlur(frame_rectangle, frame_rectangle, Size(27, 27), 1, 1);
					threshold(frame_rectangle, frame_rectangle, 0, 255, CV_THRESH_BINARY_INV | CV_THRESH_OTSU);
					Canny(frame_rectangle, frame_rectangle, 0, 10, 3);
					dilate(frame_rectangle, frame_rectangle, Mat(), Point(-1, -1), 1, 2, 1);
					findContours(frame_rectangle, contours, 3, CV_CHAIN_APPROX_SIMPLE);
				}
				// szukaj czarne pola
				else if (k == 1) {
					cvtColor(frame, frame_rectangle, CV_BGR2GRAY);
					dilate(frame_rectangle, frame_rectangle, Mat(), Point(-1, -1), 2, 1, 1);
					GaussianBlur(frame_rectangle, frame_rectangle, Size(27, 27), 1, 1);
					threshold(frame_rectangle, frame_rectangle, 0, 255, CV_THRESH_OTSU);
					Canny(frame_rectangle, frame_rectangle, 0, 10, 3);
					dilate(frame_rectangle, frame_rectangle, Mat(), Point(-1, -1), 1, 2, 1);
					findContours(frame_rectangle, contours, 3, CV_CHAIN_APPROX_SIMPLE);
				}

				float odl_max = 0.0;
				// obliczamy punkty graniczne planszy, oraz srodek
				for (int i = 0; i < contours.size(); i++)
				{
					Point2f corners[4];
					minAreaRect(contours[i]).points(corners);
					if (odleglosc(corners[0], corners[1]) >= 15 && odleglosc(corners[0], corners[1]) <= 70 &&
						odleglosc(corners[1], corners[2]) >= 15 && odleglosc(corners[1], corners[2]) <= 70 &&
						odleglosc(corners[0], corners[1])*1.4 > odleglosc(corners[1], corners[2]) &&
						odleglosc(corners[1], corners[2])*1.4 > odleglosc(corners[0], corners[1])) {

						for (int i = 0; i < 4; i++) {
							for (int j = 0; j < contours.size(); j++)
							{
								Point2f corners2[4];
								minAreaRect(contours[j]).points(corners2);
								if (odleglosc(corners2[0], corners2[1]) >= 15 && odleglosc(corners2[0], corners2[1]) <= 70 &&
									odleglosc(corners2[1], corners2[2]) >= 15 && odleglosc(corners2[1], corners2[2]) <= 70 &&
									odleglosc(corners2[0], corners2[1])*1.4 > odleglosc(corners2[1], corners2[2]) &&
									odleglosc(corners2[1], corners2[2])*1.4 > odleglosc(corners2[0], corners2[1])) {

									for (int j = 0; j < 4; j++) {
										if (odleglosc(corners[i], corners2[j]) > odl_max) {
											odl_max = odleglosc(corners[i], corners2[j]);
											rogi[(2 * k + 0)] = corners[i];
											rogi[(2 * k + 1)] = corners2[j];
										}
									}

								}
							}
						}
					}
				}
			}
			for (int i = 0; i < 4; i++) {
				if (odleglosc(rogi[i], Point2f(0.0, 0.0)) < odleglosc(lewy_gora, Point2f(0.0, 0.0)))
					lewy_gora = rogi[i];
				else if (odleglosc(rogi[i], Point2f(0.0, 480.0)) < odleglosc(lewy_dol, Point2f(0.0, 480.0)))
					lewy_dol = rogi[i];
				else if (odleglosc(rogi[i], Point2f(640.0, 480.0)) < odleglosc(prawy_dol, Point2f(640.0, 480.0)))
					prawy_dol = rogi[i];
				else if (odleglosc(rogi[i], Point2f(640.0, 0.0)) < odleglosc(prawy_gora, Point2f(640.0, 0.0)))
					prawy_gora = rogi[i];
			}
			sprawdz_plansze = false;
		}

		srodek = punkt_przeciecia(lewy_gora, prawy_dol, lewy_dol, prawy_gora);

		// rysowanie lini granicznych planszy
		line(frame, lewy_gora, prawy_gora, Scalar(0, 255, 0), 2);
		line(frame, lewy_gora, lewy_dol, Scalar(0, 255, 0), 2);
		line(frame, lewy_dol, prawy_dol, Scalar(0, 255, 0), 2);
		line(frame, prawy_dol, prawy_gora, Scalar(0, 255, 0), 2);
		line(frame, lewy_dol, prawy_gora, Scalar(0, 150, 100));
		line(frame, prawy_dol, lewy_gora, Scalar(0, 150, 100));
		circle(frame, srodek, 3, Scalar(0, 0, 255), CV_FILLED);

		// obliczanie œrodków poszczególnych pól i ich kolorów, oraz wyszukiwanie koloru pustych pól
		GaussianBlur(frame_copy, frame_copy, Size(81, 81), 1, 1);
		Point2f srodek_pola[8][8];
		Vec3b kolor_pola[8][8];
		Vec3b bialy = (128, 128, 128), czarny = (128, 128, 128);
		for (int i = 0; i < 8; i++){
			for (int j = 0; j < 8; j++) {
				srodek_pola[i][j].y = lewy_gora.y*((7 - (float)j) / 7) + prawy_gora.y*((float)j / 7) + (((lewy_dol.y - lewy_gora.y) / 8)* ((7 - (float)j) / 7) + ((prawy_dol.y - prawy_gora.y) / 8)* ((float)j / 7))*(i)* (odleglosc(prawy_gora, lewy_gora) / odleglosc(lewy_dol, prawy_dol) + ((1 - odleglosc(prawy_gora, lewy_gora) / odleglosc(lewy_dol, prawy_dol)) / 8)*(i)) + (((lewy_dol.y - lewy_gora.y) / 8)* ((7 - (float)j) / 7) + ((prawy_dol.y - prawy_gora.y) / 8)* ((float)j / 7))*(odleglosc(prawy_gora, lewy_gora) / odleglosc(lewy_dol, prawy_dol) + ((1 - odleglosc(prawy_gora, lewy_gora) / odleglosc(lewy_dol, prawy_dol)) / 8)*(i + 1))/2;
				srodek_pola[i][j].x = lewy_gora.x*((7 - (float)i) / 7) + lewy_dol.x*((float)i / 7) + (((prawy_gora.x - lewy_gora.x) / 8)* ((7 - (float)i) / 7) + ((prawy_dol.x - lewy_dol.x) / 8)*((float)i / 7))*(j)* (odleglosc(lewy_dol, lewy_gora) / odleglosc(prawy_dol, prawy_gora) + ((1 - odleglosc(lewy_dol, lewy_gora) / odleglosc(prawy_dol, prawy_gora)) / 8)*(j)) + (((prawy_gora.x - lewy_gora.x) / 8)* ((7 - (float)i) / 7) + ((prawy_dol.x - lewy_dol.x) / 8)*((float)i / 7))*(odleglosc(lewy_dol, lewy_gora) / odleglosc(prawy_dol, prawy_gora) + ((1 - odleglosc(lewy_dol, lewy_gora) / odleglosc(prawy_dol, prawy_gora)) / 8))/2;
				
				if (srodek_pola[i][j].x >= 0 && srodek_pola[i][j].x <= 640 && srodek_pola[i][j].y >= 0 && srodek_pola[i][j].y <= 480)
				kolor_pola[i][j] = frame_copy.at<Vec3b>(srodek_pola[i][j]);
				else {
					kolor_pola[i][j] = (0, 0, 0);
				}
				
				if ((kolor_pola[i][j][0] + kolor_pola[i][j][1] + kolor_pola[i][j][2]) > (bialy[0] + bialy[1] + bialy[2]))
					bialy = kolor_pola[i][j];
				else if ((kolor_pola[i][j][0] + kolor_pola[i][j][1] + kolor_pola[i][j][2]) < (czarny[0] + czarny[1] + czarny[2]))
					czarny = kolor_pola[i][j];

				circle(frame, srodek_pola[i][j], 4, Scalar(kolor_pola[i][j][0], kolor_pola[i][j][1], kolor_pola[i][j][2]), CV_FILLED);
				circle(frame, srodek_pola[i][j], 1, Scalar(0, 0, 255), CV_FILLED);
				circle(frame, srodek_pola[i][j], 4, Scalar(255-kolor_pola[i][j][0], 255-kolor_pola[i][j][1], 255-kolor_pola[i][j][2]));
			}
		}

		// wyszukiwanie i kwalifikowanie pionków na planszy
		frame_copy.convertTo(frame_copy, -1, 5, 0);
		frame_copy = frame_copy + Scalar(-120, -120, -120);
		GaussianBlur(frame_copy, frame_copy, Size(81, 81), 1, 1);
		bool od_bialego = true;
		Vec3b kolor_a = (0, 0, 0), kolor_b = (0, 0, 0);
		for (int i = 0; i < 8; i++){
			for (int j = 0; j < 8; j++) {
				if ((kolor_pola[i][j][0] + kolor_pola[i][j][1] + kolor_pola[i][j][2] + 80) >(bialy[0] + bialy[1] + bialy[2])) {
					circle(frame, srodek_pola[i][j], 8, Scalar(0, 0, 0));
					if (i == 0 && j == 7)
						od_bialego = false;
					else if (i == 7 && j == 0)
						od_bialego = false;
				}
				else if ((kolor_pola[i][j][0] + kolor_pola[i][j][1] + kolor_pola[i][j][2] - 50) < (czarny[0] + czarny[1] + czarny[2])) {
					circle(frame, srodek_pola[i][j], 8, Scalar(255, 255, 255));
					if (i == 0 && j == 0)
						od_bialego = false;
					else if (i == 7 && j == 7)
						od_bialego = false;
				}
				else {
					kolor_pola[i][j] = frame_copy.at<Vec3b>(Point(srodek_pola[i][j].x, srodek_pola[i][j].y));
					if ((kolor_a[0] + kolor_a[1] + kolor_a[2]) == 0)
						kolor_a = kolor_pola[i][j];
					if (((kolor_pola[i][j][0] + prog) > kolor_a[0] && (kolor_pola[i][j][0] - prog) < kolor_a[0]) && 
						((kolor_pola[i][j][1] + prog) > kolor_a[1] && (kolor_pola[i][j][1] - prog) < kolor_a[1]) &&
						((kolor_pola[i][j][2] + prog) > kolor_a[2] && (kolor_pola[i][j][2] - prog) < kolor_a[2])) {
						pole[j][i] = 1;
						circle(frame, srodek_pola[i][j], 16, Scalar(kolor_a[0], kolor_a[1], kolor_a[2]), 2);
					}
					else {
						if ((kolor_b[0] + kolor_b[1] + kolor_b[2]) == 0)
							kolor_b = kolor_pola[i][j];
						pole[j][i] = -1;
						circle(frame, srodek_pola[i][j], 16, Scalar(kolor_b[0], kolor_b[1], kolor_b[2]), 2);
					}
				}
			}
		}

		// rysujemy planszê wraz z pionkami
		for (int i = 0; i < 8; i++) {
			for (int j = 0; j < 8; j++) {
				if (od_bialego)
					rectangle(plansza, Point(20 + (50 * i), 20 + (50 * j)), Point(70 + (50 * i), 70 + (50 * j)), Scalar(255 * ((i + j + 1) % 2), 255 * ((i + j + 1) % 2), 255 * ((i + j + 1) % 2)), CV_FILLED);
				else 
					rectangle(plansza, Point(20 + (50 * i), 20 + (50 * j)), Point(70 + (50 * i), 70 + (50 * j)), Scalar(255 * ((i + j) % 2), 255 * ((i + j) % 2), 255 * ((i + j) % 2)), CV_FILLED);
				if (pole[i][j] > 0) {
					circle(plansza, Point(45 + 50 * i, 45 + 50 * j), 20, Scalar(kolor_a[0], kolor_a[1], kolor_a[2]), CV_FILLED);
					gracz1++;
				}
				else if (pole[i][j] < 0) {
					circle(plansza, Point(45 + 50 * i, 45 + 50 * j), 20, Scalar(kolor_b[0], kolor_b[1], kolor_b[2]), CV_FILLED);
					gracz2++;
				}
			}
		} rectangle(plansza, Point(20, 20), Point(420, 420), Scalar(200,200,200));
		putText(plansza, "Gracz 1: "+to_string(gracz1), cvPoint(20, 455), FONT_HERSHEY_COMPLEX_SMALL, 1, cvScalar(255, 255, 255), 1, CV_AA);
		putText(plansza, "Gracz 2: "+to_string(gracz2), cvPoint(280, 455), FONT_HERSHEY_COMPLEX_SMALL, 1, cvScalar(255, 255, 255), 1, CV_AA);

		imshow("Plansza", plansza);
		imshow("Regulacja", frame);

		counter++;
		if (counter >= 10) {
			counter = 0;
			sprawdz_plansze = true;
		}

		if (waitKey(30) == 27) //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
		{
			cout << "esc key is pressed by user" << endl;
			break;
		}
	}
	return 0;

}