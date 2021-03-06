#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <math.h>

#define RADIUS 20

using namespace cv;
using namespace std;

Mat filter, tmp;
int dft_N, dft_M;
int dh_slider = 20;
int dh_slider_max = 100;

int dl_slider = 5;
int dl_slider_max = 100;

int c_slider = 5;
int c_slider_max = 100;

int d0_slider = 80;
int d0_slider_max = 1000;

char TrackbarName[50];

void slider(int, void*){
	int M,N;
	float D2, dh, dl, d0;
	M = dft_M;
	N = dft_N;
	dh = dh_slider/10.0;
	dl = dl_slider/10.0;
	d0 = d0_slider/10.0;
  //calculando o filtro homomorfico a partir do ajuste dos sliders da trackbar. 
	tmp = Mat(dft_M, dft_N, CV_32F);
	for(int i=0; i<dft_M ;i++)
		for(int j=0; j<dft_N ;j++){
				D2 = ((float)i-M/2.0)*((float)i-M/2.0) + ((float)j-N/2.0)*((float)j-N/2.0);
				tmp.at<float>(i,j) = (dh-dl)*(1.0-exp(-1.0*(float)c_slider*(D2/(d0*d0))))+ dl;
			}
	
  // cria a matriz com as componentes do filtro e junta
  // ambas em uma matriz multicanal complexa
  Mat comps[]= {tmp, tmp};
  merge(comps, 2, filter);

}

// troca os quadrantes da imagem da DFT
void deslocaDFT(Mat& image ){
  Mat A, B, C, D;

  // se a imagem tiver tamanho impar, recorta a regiao para
  // evitar cÃƒÂ³pias de tamanho desigual
  image = image(Rect(0, 0, image.cols & -2, image.rows & -2));
  int cx = image.cols/2;
  int cy = image.rows/2;
  
  // reorganiza os quadrantes da transformada
  // A B   ->  D C
  // C D       B A
  A = image(Rect(0, 0, cx, cy));
  B = image(Rect(cx, 0, cx, cy));
  C = image(Rect(0, cy, cx, cy));
  D = image(Rect(cx, cy, cx, cy));

  // A <-> D
  A.copyTo(tmp);  D.copyTo(A);  tmp.copyTo(D);

  // C <-> B
  C.copyTo(tmp);  B.copyTo(C);  tmp.copyTo(B);
}

int main(int argc , char** argv){
  VideoCapture cap;   
  Mat imaginaryInput, complexImage, multsp;
  Mat padded, mag;
  Mat image, imagegray; 
  Mat_<float> realInput, zeros;
  vector<Mat> planos;

  // guarda tecla capturada
  char key;
	if(argc != 2){
		printf("ERRO\n");
		exit(-1);
	}
	image = imread(argv[1],CV_LOAD_IMAGE_GRAYSCALE); // carrega a imagem

	cv::log(realInput, realInput);
  // identifica os tamanhos otimos para calculo da FFT
  dft_M = getOptimalDFTSize(image.rows);
  dft_N = getOptimalDFTSize(image.cols);

  // realiza o padding da imagem
  copyMakeBorder(image, padded, 0,
                 dft_M - image.rows, 0,
                 dft_N - image.cols,
                 BORDER_CONSTANT, Scalar::all(0));
	
  // parte imaginaria da matriz complexa (preenchida com zeros)
  zeros = Mat_<float>::zeros(padded.size());

  // prepara a matriz complexa para ser preenchida
  complexImage = Mat(padded.size(), CV_32FC2, Scalar(0));

  filter = complexImage.clone();	
	slider(1,0);
	
  for(;;){

    planos.clear();

    realInput = Mat_<float>(padded); 

    planos.push_back(realInput);
    planos.push_back(zeros);

    merge(planos, complexImage);

    dft(complexImage, complexImage);

    deslocaDFT(complexImage);

    mulSpectrums(complexImage,filter,complexImage,0);

    deslocaDFT(complexImage);

    idft(complexImage, complexImage);

    planos.clear();

  
    split(complexImage, planos);


    normalize(planos[0], planos[0], 0, 1, CV_MINMAX);
		// calcula a exponencial da imagem
		cv::exp(planos[0], planos[0]);

    normalize(planos[0], planos[0], 0, 1, CV_MINMAX);
    imshow("filtrada", planos[0]);

		key = (char) waitKey(10);
    if( key == 27 ) break; // esc pressed!
 
  	sprintf( TrackbarName, "H %d", dh_slider_max/10);
  	createTrackbar( TrackbarName, "filtrada",
				  &dh_slider,
				  dh_slider_max,
				  NULL); //funcao

		sprintf( TrackbarName, "L %d", dl_slider_max/10);
  	createTrackbar( TrackbarName, "filtrada",
				  &dl_slider,
				  dl_slider_max,
				  NULL); //funcao

		sprintf( TrackbarName, "C %d", c_slider_max/10);
  	createTrackbar( TrackbarName, "filtrada",
				  &c_slider,
				  c_slider_max,
				  NULL); //funcao

		sprintf( TrackbarName, "D0 %d", d0_slider_max/10);
  	createTrackbar( TrackbarName, "filtrada",
				  &d0_slider,
				  d0_slider_max,
				  NULL); //funcao 

		slider(d0_slider,0);
  }
  return 0;
}
