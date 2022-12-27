#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
     

typedef struct 
{
	__int16 fileTipe;   				// Тип файла, символы BM
	int fileSize;   				// Размер файла в байтах 
	int fileReserve;             			// Резервная область
	int offset;                  			// Смещение до самого изображения 
	int imageSize;                                  // Размер структуры в байтах
	int imageWidth;                                 // Ширина изображения в пикселях
	int imageHeight;                                // Высота изображения в пикселях
   	__int16 planesCount;                            // Количество цветовых плоскостей
   	__int16 bitsOnPixelCount;                       // Количество бит на пиксель
   	int compression;                 		// Тип сжатия для сжатых изображений
   	int bitCount;                     		// Количество байт в поле данных
	int xResolution;          			// Горизонтальное разрешение в пикселях на метр
	int yResolution;          			// Вертикальное разрешение в пикселях на метр
   	int colorsCount;                              	// Количество используемых цветов в палитре
   	int importantcolors;                		// Количество элементов палитры, необходимых для отображения изображения
}__attribute__((__packed__)) 
BMPHEADER;

std:: string* ReadIniFile(std::string *iniData)
{
	std::ifstream ini;
	ini.open("ini.txt");
	
	if (!ini.is_open())
	{
		std::cout << ".ini file opening error! Please, check for .ini file existence.";
		exit(0);
	}
	else
	{
		for (int k = 0; k < 4; k++)
		{
			ini >> iniData[k];
			if (iniData[k] == "")
				{
					std::cout << "The ini file does not match!";
					exit(0);
				}
		}

		int videoWidth = atoi(iniData[2].c_str());
		if (videoWidth < 0)
		{
			std::cout << "The width of the image cannot be negative!";
			exit(0);
		}

		int videoHeight = atoi(iniData[3].c_str());
		if (videoHeight < 0)
		{
			std::cout << "The height of the image cannot be negative!";
			exit(0);
		}
	}

	return iniData;
}

int GetBufferSize(FILE *file) 
{
	int size;
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	return size;
}

void OverlayImage(unsigned char *y, unsigned char *u, unsigned char *v, const char* yuvVideo, int videoWidth, int videoHeight)
{
	FILE *video = fopen(yuvVideo, "r+w");
	if (video == NULL) 
	{
        	std::cout << "Video failed to load!";
                exit(0);
        }
	
	int imageResolution = videoHeight * videoWidth;

	unsigned char *yVideo, *uVideo, *vVideo;
	yVideo = (unsigned char *)malloc(imageResolution);
	uVideo = (unsigned char *)malloc(imageResolution / 4);
	vVideo = (unsigned char *)malloc(imageResolution / 4);

	int bufferVideoSize = GetBufferSize(video);
	int frameCount = bufferVideoSize / (imageResolution + imageResolution / 2);

	std::cout << "Video size: " << bufferVideoSize << "\n";		
	std::cout << "Video frame count: " << frameCount << "\n";

	for (int i = 0; i < imageResolution; i++)
	{
		*(yVideo + i) = *(y + i);
	}

	for (int i = 0; i < imageResolution / 4; i++)
	{
		*(uVideo + i) = *(u + i);
		*(vVideo + i) = *(v + i);
	}

	for (int i = 0; i < frameCount ; i++)
	{
		fwrite(yVideo, 1, imageResolution, video);
		fwrite(uVideo, 1, imageResolution / 4, video);
		fwrite(vVideo, 1, imageResolution / 4, video);
	}

	free(yVideo);
	free(uVideo);
	free(vVideo);
	free(y);
	free(u);
	free(v);

	fclose(video);
}

void ConvertingBmp(const char* bmpImage, const char* yuvVideo, int videoWidth, int videoHeight)
{
        BMPHEADER bmpHeader;

	FILE *bmp = fopen (bmpImage, "rb");
	if (bmp == NULL) 
	{
		std::cout << "Image failed to load!";
                exit(0);
        }

    	fread(&bmpHeader, sizeof(BMPHEADER), 1, bmp);

	int width = bmpHeader.imageWidth;
	int height = bmpHeader.imageHeight;
	int imageResolution = width * height;

	std::cout << "Width BMP image: " << width << "\n";
	std::cout << "Height BMP image: " << height << "\n";

	int bmpLineLength = (3 * width + 3) & (-4); // находим длину строки в файле
	int *r, *g, *b;
	unsigned char *y, *u, *v, *uTemp, *vTemp, *buffer;

	r = (int *)malloc(imageResolution * 4);
	g = (int *)malloc(imageResolution * 4);
	b = (int *)malloc(imageResolution * 4);
	y = (unsigned char *)malloc(imageResolution);
	uTemp = (unsigned char *)malloc(imageResolution);
	vTemp = (unsigned char *)malloc(imageResolution);
	u = (unsigned char *)malloc(imageResolution / 4);
	v = (unsigned char *)malloc(imageResolution / 4);
    	buffer = (unsigned char *)malloc(bmpLineLength*height);
	
    	fseek( bmp, 0, SEEK_END);
    	fseek( bmp, sizeof(BMPHEADER) + 84, SEEK_SET);
	fread( buffer, 1, bmpLineLength * height, bmp);
	fclose(bmp);

    	for(int y = height - 1, k = 0; y >= 0; y--) // считывание RGB в правильном виде
	{
        	unsigned char *pRow = buffer + bmpLineLength * y;
        	for(int x = 0; x < width; x++, k++) 
		{
            		*(r + k) = *(pRow + 2);
            		*(g + k) = *(pRow + 1);
            		*(b + k) = *pRow; 
            		pRow+=3;
        	}
    	}

	for (int i = 0; i < imageResolution; i++) // Тут ипользовал базовую формулу RGB -> YUV сразу с квантованием
	{
		*(y + i) = *(r + i) * 0.256788 + *(g + i) * 0.504129 + *(b + i) * 0.097906 + 16;
		*(uTemp + i) = *(r + i) * (-0.148223) - *(g + i) * 0.290993 + *(b + i) * 0.439216 + 128;
		*(vTemp + i) = *(r + i) * 0.439216 - *(g + i) * 0.367788 - *(b + i) * 0.071427 + 128;
	}
	
	for (int i = 0, k = 0; i < height / 2; i++) // Тут ипользовал найденую формулу YUV 444 -> YUV 420, с помощью даунсэмплинга U и V
	{
		for (int j = 0; j < width / 2; j++, k++)
		{
			*(u + k) = (*(uTemp + width * i * 2 + 2 * j) + *(uTemp + width * i * 2 + 2 * j + 1) + *(uTemp + width  * (2 * i + 1) + 2 * j) + *(uTemp + width  * (2 * i + 1) + 2 * j + 1)) / 4;
			*(v + k) = (*(vTemp + width * i * 2 + 2 * j) + *(vTemp + width * i * 2 + 2 * j + 1) + *(vTemp + width  * (2 * i + 1) + 2 * j) + *(vTemp + width  * (2 * i + 1) + 2 * j + 1)) / 4;
		}
	}

	free(r);
	free(g);
	free(b);
	free(uTemp);
	free(vTemp);
	free(buffer);

	OverlayImage(y, u, v, yuvVideo, videoWidth, videoHeight);
}

int main()
{
	std::string iniData[4];
	ReadIniFile(iniData);

	const char* bmpImage = iniData[0].c_str(); 
	const char* yuvVideo = iniData[1].c_str(); 
	int videoWidth = atoi(iniData[2].c_str());
	int videoHeight = atoi(iniData[3].c_str());

	ConvertingBmp(bmpImage, yuvVideo, videoWidth, videoHeight);

	std::cout << "Runtime: " << clock()/1000.0 << " sec.";
	return 0;
}