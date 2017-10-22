#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>                                                                                                                                                                                                                                                                                                       
#include "paris_llf.h"
#include "Image.h"
#include "fast_blur.h"
#include <time.h>
#include "pgm.c"





static inline int intDivUp(int a, int b) {
  return (a % b != 0) ? (a / b + 1) : (a / b);
}


const int  J = 5;
#ifndef max
  #define max(a, b) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef min
  #define min(a, b) ( ((a) < (b)) ? (a) : (b) )
#endif

double SmoothStep(double x_min, double x_max, double x) {
  double y = (x - x_min) / (x_max - x_min);
  y = max(0.0, min(1.0, y));
  return pow(y, 2) * pow(y-2, 2);
}

double DetailRemap(double delta, double sigma_r) {
  double fraction = delta / sigma_r;
  double polynomial = pow(fraction, alpha_);
  if (alpha_ < 1) {
    const double kNoiseLevel = 0.01;
    double blend = SmoothStep(kNoiseLevel,2 * kNoiseLevel, fraction * sigma_r);
    polynomial = blend * polynomial + (1 - blend) * fraction;
  }
  return polynomial;
}

double EdgeRemap(double delta) {
  return beta_ * delta;
}
void RemappingFunction(double value,
                                 double reference,
                                 double sigma_r,
                                 int output_dst,
                                 pixel_t * dst) {
  	double delta = reference - value;
  	if(value>reference)
  		delta = value - reference;
  	
  	int sign = value < reference ? -1 : 1;
  	
	double temp;
  	if (delta < sigma_r) {
    	temp = reference + sign * sigma_r * DetailRemap(delta, sigma_r);     	

  	} else {
    	temp = reference + sign * (EdgeRemap(delta - sigma_r) + sigma_r);	
  	}
  	dst[output_dst] = (pixel_t)temp;
  
}
void remapp(range row_range ,range col_range ,int col_size, int row_size, pixel_t  * dst ,pixel_t reference ,double sigma){
	int x,y;

	pixel_t input;
	for (y = 0; y < row_size ; ++y)
	{
		for (x = 0; x < col_size; ++x)
		{	
		//printf("%d , %d \n",col_range.start +x,y+row_range.start );
			int output_dst = x+y*col_size;
			if ((y+row_range.start)>=im.h||(x+col_range.start)>=im.w||(x+col_range.start)<0||(y+row_range.start)<0)
			{
				input=0;
				//dst[output_dst]=0;
			}
			else
			{
				input = pim[ col_range.start +x +(y+row_range.start)*im.w];
				// RemappingFunction((double)input,(double)reference,sigma,output_dst,dst);
				//dst[output_dst] = input;
			}
			RemappingFunction((double)input,(double)reference,sigma,output_dst,dst);
			//dst[output_dst] = input;
		}
	}
}

pixel_t *  paris_llf(int img_w,int img_h,pixel_t *image, double alpha, double beta,double sigma){
  	
	//int * global_pointer;
    alpha_=alpha;
    beta_ =beta ;
    im.w = img_w;
    im.h = img_h;
    im.img = 0;
    pim = image;
    
    int j,i,k;
	
	/////////////////////////////////////////////////////////////////////address manager ///////////////////////////////////
    int Level_w[J];
    int Level_h[J];
    int Level_size[J];
    // compute the size of the image at every pyramid level 
    int pyramid_size ;
    Level_w[0] = im.w;
    Level_h[0] = im.h;
    Level_size[0] = im.w*im.h;
    double time_elapsed=0;
	remapp_time_elapsed=0;
	blur_time_elapsed=0;
	upsample_time_elapsed=0;
	downsample_time_elapsed=0;
	subtract_time_elapsed=0;

printf("%d %d \n", Level_w[0],Level_h[0]);
    pyramid_size = Level_size[0];
    for (i = 1; i < J; ++i)
    {
        Level_w[i] = Level_w[i-1]/2+Level_w[i-1]%2;
        Level_h[i] = Level_h[i-1]/2+Level_h[i-1]%2;
        printf("%d %d \n", Level_w[i],Level_h[i]);
        Level_size[i] = Level_w[i]*Level_h[i];
        pyramid_size += Level_size[i];
    }

	// compute addresses for input pyramids//////////////////////////////////////////////////////////////////////////////////////////////////

    //Gaussian/////////////////////
	    Image imGPyramid[J];
	   


	    imGPyramid[0].img = pim;
	    imGPyramid[0].w = Level_w[0];
	    imGPyramid[0].h = Level_h[0];
	    //pimGPyramid[0] = pim;//(double *) malloc(Level_size[0]*sizeof(double));
	    for (i = 1; i < J; ++i)
	    {
	     
	       imGPyramid[i].img = (pixel_t *) malloc(Level_size[i]*sizeof(pixel_t));
	       imGPyramid[i].w = Level_w[i];
	       imGPyramid[i].h = Level_h[i];
	    }
	///////////////////////////////
printf("malloc for Gaussian Pyramid done\n");

    //temp for Gaussian-Laplacian////////////
	    Image imTempPyramid[J];
	    
	    

	    for (i = 0; i < J; ++i)
	    {
 
			imTempPyramid[i].img =  (pixel_t *) malloc(Level_size[i]*sizeof(pixel_t));
	    	
	    	imTempPyramid[i].h = Level_w[i];
	    	imTempPyramid[i].w = Level_h[i];
	    }
	///////////////////////////////
printf("malloc for imTempPyramid done\n");


    //Laplacian////////////////////
	    Image imLPyramid[J];


	    for (i = 0; i < J; ++i)
	    {
   	    	imLPyramid[i].img =  (pixel_t *) malloc(Level_size[i]*sizeof(pixel_t));
	       	imLPyramid[i].w = Level_w[i];
	       	imLPyramid[i].h = Level_h[i];
	    }
    //////////////////////////////
printf("malloc for Laplacian pyramid done\n");


struct timespec time_start, time_finish;
 
	    

   

	/////////////////////////////////////////////////////////////////////////

 
	int cstart =0;
	int rstart =0;
    // Compute a Gaussian  pyramid 



    for (j = 1; j < J; j++) {
        PyramidDown(imGPyramid[j-1].w, imGPyramid[j-1].h, imGPyramid[j-1].img, imTempPyramid[j-1].img , imGPyramid[j].img,0,0);
        //PyramidUp_sub( 	imGPyramid[j].w  , imGPyramid[j].h  , imGPyramid[j].img  , imTempPyramid[j-1].img , imLPyramid[j-1].img , imGPyramid[j-1].img,0,0,imGPyramid[j-1].w  , imGPyramid[j-1].h );
	}
    for (i = 0; i < imLPyramid[J-1].w*imLPyramid[J-1].h; ++i)
    {
    	imLPyramid[J-1].img[i] = imGPyramid[J-1].img[i];
    }


	printf("Gaussian pyramid done\n");
    Image Pyramid[J];
	Image local_GPyramid[J];
	Image local_LPyramid[J];
	Image local_TPyramid[J];
	int hwt = 3*(1<<(J-1))-2;
	Pyramid[0].w = 2*hwt;
	Pyramid[0].h = 2*hwt;

	local_GPyramid[0].img = malloc((4*hwt*hwt)*sizeof(pixel_t));
	local_LPyramid[0].img = malloc((4*hwt*hwt)*sizeof(pixel_t));
	local_TPyramid[0].img = malloc((4*hwt*hwt)*sizeof(pixel_t));
	//pixel_t * ones = malloc((4*hw*hw)*sizeof(pixel_t));
	//printf("%d vs %d  \n",Pyramid[0].w ,2*hw );

	for (i = 1; i < J; ++i)
	{
		Pyramid[i].h = Pyramid[i-1].h/2+Pyramid[i-1].h%2;
		Pyramid[i].w = Pyramid[i-1].w/2+Pyramid[i-1].w%2;
		
		local_GPyramid[i].img = malloc(Pyramid[i].h*Pyramid[i].w*sizeof(pixel_t));
		local_LPyramid[i].img = malloc(Pyramid[i].h*Pyramid[i].w*sizeof(pixel_t));
		local_TPyramid[i].img = malloc(Pyramid[i].h*Pyramid[i].w*sizeof(pixel_t));
	}

	double time_elapsed_t=0;
	double remapp_time_elapsed_t=0;
	double blur_time_elapsed_t=0;
	double upsample_time_elapsed_t=0;
	double downsample_time_elapsed_t=0;
	double subtract_time_elapsed_t=0;
	clock_gettime(CLOCK_MONOTONIC, &time_start);    	

    
    for (int l = 0; l < J-1; l++) {
		int hw = 3*(1<<(l+1))-2;

		Pyramid[0].w = 2*hw;
		Pyramid[0].h = 2*hw;
		local_LPyramid[0].h = 2*hw;
   		local_LPyramid[0].w = 2*hw;
   		local_GPyramid[0].h = 2*hw;
   		local_GPyramid[0].w = 2*hw;

		//pixel_t * ones = malloc((4*hw*hw)*sizeof(pixel_t));
		//printf("%d vs %d  \n",Pyramid[0].w ,2*hw );

		for (i = 1; i < l+2; ++i)
		{	       				
   			Pyramid[i].h = Pyramid[i-1].h/2+Pyramid[i-1].h%2;
   			Pyramid[i].w = Pyramid[i-1].w/2+Pyramid[i-1].w%2;
   			local_LPyramid[i].h = Pyramid[i-1].h/2+Pyramid[i-1].h%2;
   			local_LPyramid[i].w = Pyramid[i-1].w/2+Pyramid[i-1].w%2;
   			
   			local_GPyramid[i].h = Pyramid[i-1].h/2+Pyramid[i-1].h%2;
   			local_GPyramid[i].w = Pyramid[i-1].w/2+Pyramid[i-1].w%2;
			

   		}
    	printf("level = %d ,hw =%d  ,last %d element %d, offset %d \n",l ,hw, hw/(1<<(l+1)), hw>>l,1<<l);
    	float tilesz = 172;
    	float jobs = (float)(tilesz-2*(float)hw)/(float)(1<<l);
    	float tpj = tilesz/jobs;
   		//printf("jobs %f , tpj %f , \n",jobs,tpj);
	    for (int y = 0; y < imLPyramid[l].h; ++y) {
	      	for (int x = 0; x < imLPyramid[l].w; ++x) {

            	int yf = y*(1<<(l)) ;
            	int xf = x*(1<<(l)) ;

				int yfclev0 = hw>>l;
		       	int xfclev0 = hw>>l;

        		
          		row_range.start = yf-hw;
            	row_range.end = yf+hw;
            	
            	col_range.start = xf-hw;
            	col_range.end = xf+hw;
				
            
	   		


				clock_gettime(CLOCK_MONOTONIC, &remapp_time_start);

				pixel_t g0 = imGPyramid[l].img[imGPyramid[l].w*y+x];
		        remapp(row_range , col_range,Pyramid[0].w,Pyramid[0].h, local_GPyramid[0].img,g0,sigma);

				clock_gettime(CLOCK_MONOTONIC, &remapp_time_finish);
				remapp_time_elapsed += (remapp_time_finish.tv_sec - remapp_time_start.tv_sec);
				remapp_time_elapsed += (remapp_time_finish.tv_nsec - remapp_time_start.tv_nsec) / 1000000000.0;

				int c_off, r_off;
				cstart = col_range.start;
				rstart = row_range.start;	
		        for (j = 1; j < l+2; j++) {
		        	c_off=abs(cstart%2);
					r_off=abs(rstart%2);
					cstart= cstart/2+c_off;
					rstart= rstart/2+r_off;
		        	PyramidDown(	Pyramid[j-1].w, Pyramid[j-1].h, local_GPyramid[j-1].img, local_TPyramid[j-1].img , local_GPyramid[j].img,c_off,r_off);
        			//PyramidUp_sub( 	Pyramid[j].w  , Pyramid[j].h  , local_GPyramid[j].img  , local_TPyramid[j-1].img , local_LPyramid[j-1].img , local_GPyramid[j-1].img,c_off,r_off,Pyramid[j-1].w  , Pyramid[j-1].h );
    			}
				PyramidUp( 	Pyramid[l+1].w  , Pyramid[l+1].h  , local_GPyramid[l+1].img  , local_TPyramid[l].img , local_LPyramid[l].img , local_GPyramid[l].img,c_off,r_off,Pyramid[l].w  , Pyramid[l].h );
 
    			//printf("x %d , y %d\n",x,y );
    			pixel_t value =local_GPyramid[l].img[Pyramid[l].w*yfclev0 + xfclev0]- local_LPyramid[l].img[Pyramid[l].w*yfclev0 + xfclev0];		       	
  				imLPyramid[l].img[imLPyramid[l].w*y+x] =value;


	      	}
	    }
	    clock_gettime(CLOCK_MONOTONIC, &time_finish); 
		
		time_elapsed = (time_finish.tv_sec - time_start.tv_sec);
		time_elapsed += (time_finish.tv_nsec - time_start.tv_nsec) / 1000000000.0;
		printf(">>>> Time  = %f ms\n",			time_elapsed*1000-time_elapsed_t*1000);
		printf(">>>> Time remap = %f ms\n",		remapp_time_elapsed*1000-remapp_time_elapsed_t*1000);
		printf(">>>> Time blur  = %f ms\n",		blur_time_elapsed*1000-blur_time_elapsed_t*1000);
		printf(">>>> Time upsample = %f ms\n",	upsample_time_elapsed*1000-upsample_time_elapsed_t*1000);
		printf(">>>> Time downsample = %f ms\n",downsample_time_elapsed*1000-downsample_time_elapsed_t*1000);
		printf(">>>> Time subtract = %f ms\n",	subtract_time_elapsed*1000-subtract_time_elapsed_t*1000);
		time_elapsed_t= time_elapsed;
		remapp_time_elapsed_t= remapp_time_elapsed;
		blur_time_elapsed_t= blur_time_elapsed;
		upsample_time_elapsed_t= upsample_time_elapsed;
		downsample_time_elapsed_t= downsample_time_elapsed;
		subtract_time_elapsed_t= subtract_time_elapsed;

	}   
	clock_gettime(CLOCK_MONOTONIC, &time_finish); 
	//double time_elapsed;
	time_elapsed = (time_finish.tv_sec - time_start.tv_sec);
	time_elapsed += (time_finish.tv_nsec - time_start.tv_nsec) / 1000000000.0;
	printf("\n\n");
	printf(">>>> Time  = %f ms\n",time_elapsed*1000);
	printf(">>>> Time remap = %f ms\n",remapp_time_elapsed*1000);
	printf(">>>> Time blur  = %f ms\n",blur_time_elapsed*1000);
	printf(">>>> Time upsample = %f ms\n",upsample_time_elapsed*1000);
	printf(">>>> Time downsample = %f ms\n",downsample_time_elapsed*1000);
	printf(">>>> Time subtract = %f ms\n",subtract_time_elapsed*1000);



    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
    // Now collapse the output laplacian pyramid
    // printf("Collapsing laplacian pyramid down to output image\n");
    //printf("pyramids done and rescaled\n");


    for (j = J-1; j > 0; j--) {
		PyramidUp_add(imLPyramid[j-1].w, imLPyramid[j-1].h, imLPyramid[j].img,imTempPyramid[j-1].img , imLPyramid[j-1].img,imLPyramid[j-1].img,0,0);
    }
    //printf("pyramid colapsed\n");
    return imLPyramid[0].img;

}