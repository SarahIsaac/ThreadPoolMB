#include <iostream>
#include<ostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <cmath>
#include <thread>
#include "Timer.h"
#include "TaskQueue.h"

float DIM = 512; //always going to be a square
float MINR = -2;
float MAXR = 1;
float MINI = -1;
float MAXI = 1;



struct Color {
	int red;
	int green;
	int blue;

	Color()
	{
		red = 0;
		green = 0;
		blue = 0;
	}

	Color(int r, int g, int b)
	{
		red = r;
		green = g;
		blue = b;
	}
};

typedef std::vector<std::vector<Color>> image;

void writeImage(image const &img, std::ofstream& o, std::string filename = "mandelbrot.ppm")
{
	o.open(filename);
	o << "P3" << std::endl;
	o << DIM << " " << DIM << std::endl;
	o << 255 << std::endl;
	for (int i = 0; i < DIM; i++)
	{
		std::vector<Color> vec = img[i];
		for (int j = 0; j < DIM; j++)
		{
			o << vec[j].red % 2 << " " << vec[j].green << " " << vec[j].blue << " ";
		}
		o << std::endl;
	}
	o.close();
}

Color determineColor(int iterations)
{
	Color c(iterations, iterations, iterations);
	return c;
}

int doMandelbrot(float x_a, float y_a)
{
	float x = 0;
	float y = 0;
	int i = 0;
	while (i < 256 && (x*x) + (y*y) < (2 * 2))
	{
		float temp = x*x - y*y + x_a;
		y = (2 * x * y) + y_a;
		x = temp;
		i++;
	}
	return i;
}

double getAverage(std::vector<double> times)
{
	int size = times.size();
	double total = 0;
	for (int i = 0; i < times.size(); i++)
	{
		total += times[i];
	}
	double average = total / size;
	return average;
}

double getStdDev(double average, std::vector<double> times)
{
	double size = times.size();
	double sum = 0;
	for (double i = 0; i < times.size(); i++)
	{
		sum += ((times[i] - average) * (times[i] - average));
	}
	sum = sqrt(sum / size);
	return sum;
}

void MandelBrotInnards(float x, float y, int a, int b, image &img)
{
	int iteration = doMandelbrot(x, y);
	Color color = determineColor(iteration);
	img[a][b] = color;
}

image ThreadPoolByPixel(TaskQueue &q)
{
	std::vector<std::vector<Color>> img(DIM, std::vector<Color>(DIM));
	{
		for (int a = 0; a < DIM; a++)
		{
			float x = ((a / DIM) * (MAXR - MINR)) + MINR;
			for (int b = 0; b < (int)DIM; b++)
			{
				float y = ((b / DIM) * (MAXI - MINI)) + MINI;
				q.add_task([a, b, x, y, &img] {MandelBrotInnards(x, y, a, b, img); });
			}
		}
	}
	return img;
}

void MandelBrotRow(float a, float x, image &img)
{
	for (float b = 0; b < DIM; b++)
	{
		float y = ((b / DIM) * (MAXI - MINI)) + MINI;
		int iteration = doMandelbrot(x, y);
		Color color = determineColor(iteration);
		img[a][b] = color;
	}
}

image ThreadPoolByRow(TaskQueue &q)
{
	std::vector<std::vector<Color>> img(DIM, std::vector<Color>(DIM));
	{
		for (int a = 0; a < DIM; a++)
		{
			float x = ((a / DIM) * (MAXR - MINR)) + MINR;
			q.add_task([a, x, &img] {MandelBrotRow(a, x, img); });
		}
	}
	return img;
}

int main()
{
	TaskQueue q(4);

	image img = ThreadPoolByPixel(q);
	std::ofstream os;
	std::string filename = "threadpool_mandel.ppm";
	writeImage(img, os, filename);

	std::vector<double> times;
	for (int i = 0; i < 5; i++)
	{
		q.set_task_size(512 * 512);
		double time = functionTimer([&q]()->void {ThreadPoolByPixel(q); });
		times.push_back(time);
	}

	double average = getAverage(times);
	double std_dev = getStdDev(average, times);
	std::cout << "ThreadPool w/ Tasks divided up by PIXEL" << std::endl;
	std::cout << "Average time (in milliseconds): " << average << std::endl;
	std::cout << "Standard Deviation: " << std_dev << std::endl;

	//couldn't get this to work, ran out of time to work on it.
	//for (int i = 0; i < 5; i++)
	//{
	//	q.set_task_size(512);
	//	ThreadPoolByRow(q);
	//	std::cout << "done2 " << i << std::endl;
	//}

	q.join();

	return 0;
}
