/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/
#include "_cv.h"

#define ICV_DIST_SHIFT  16
#define ICV_INIT_DIST0  (INT_MAX >> 2)

static CvStatus
icvInitTopBottom( int* temp, int tempstep, CvSize size, int border )
{
    int i, j;
    for( i = 0; i < border; i++ )
    {
        int* ttop = (int*)(temp + i*tempstep);
        int* tbottom = (int*)(temp + (size.height + border*2 - i - 1)*tempstep);
        
        for( j = 0; j < size.width + border*2; j++ )
        {
            ttop[j] = ICV_INIT_DIST0;
            tbottom[j] = ICV_INIT_DIST0;
        }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvDistanceTransform_3x3_C1R( const uchar* src, int srcstep, int* temp,
        int step, float* dist, int dststep, CvSize size, const float* metrics )
{
    const int BORDER = 1;
    int i, j;
    const int HV_DIST = CV_FLT_TO_FIX( metrics[0], ICV_DIST_SHIFT );
    const int DIAG_DIST = CV_FLT_TO_FIX( metrics[1], ICV_DIST_SHIFT );
    const float scale = 1.f/(1 << ICV_DIST_SHIFT);

    srcstep /= sizeof(src[0]);
    step /= sizeof(temp[0]);
    dststep /= sizeof(dist[0]);

    icvInitTopBottom( temp, step, size, BORDER );

    // forward pass
    for( i = 0; i < size.height; i++ )
    {
        const uchar* s = src + i*srcstep;
        int* tmp = (int*)(temp + (i+BORDER)*step) + BORDER;

        for( j = 0; j < BORDER; j++ )
            tmp[-j-1] = tmp[size.width + j] = ICV_INIT_DIST0;
        
        for( j = 0; j < size.width; j++ )
        {
            if( !s[j] )
                tmp[j] = 0;
            else
            {
                int t0 = tmp[j-step-1] + DIAG_DIST;
                int t = tmp[j-step] + HV_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-step+1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-1] + HV_DIST;
                if( t0 > t ) t0 = t;
                tmp[j] = t0;
            }
        }
    }

    // backward pass
    for( i = size.height - 1; i >= 0; i-- )
    {
        float* d = (float*)(dist + i*dststep);
        int* tmp = (int*)(temp + (i+BORDER)*step) + BORDER;
        
        for( j = size.width - 1; j >= 0; j-- )
        {
            int t0 = tmp[j];
            if( t0 > HV_DIST )
            {
                int t = tmp[j+step+1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step] + HV_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step-1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+1] + HV_DIST;
                if( t0 > t ) t0 = t;
                tmp[j] = t0;
            }
            d[j] = (float)(t0 * scale);
        }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvDistanceTransform_5x5_C1R( const uchar* src, int srcstep, int* temp,
        int step, float* dist, int dststep, CvSize size, const float* metrics )
{
    const int BORDER = 2;
    int i, j;
    const int HV_DIST = CV_FLT_TO_FIX( metrics[0], ICV_DIST_SHIFT );
    const int DIAG_DIST = CV_FLT_TO_FIX( metrics[1], ICV_DIST_SHIFT );
    const int LONG_DIST = CV_FLT_TO_FIX( metrics[2], ICV_DIST_SHIFT );
    const float scale = 1.f/(1 << ICV_DIST_SHIFT);

    srcstep /= sizeof(src[0]);
    step /= sizeof(temp[0]);
    dststep /= sizeof(dist[0]);

    icvInitTopBottom( temp, step, size, BORDER );

    // forward pass
    for( i = 0; i < size.height; i++ )
    {
        const uchar* s = src + i*srcstep;
        int* tmp = (int*)(temp + (i+BORDER)*step) + BORDER;

        for( j = 0; j < BORDER; j++ )
            tmp[-j-1] = tmp[size.width + j] = ICV_INIT_DIST0;
        
        for( j = 0; j < size.width; j++ )
        {
            if( !s[j] )
                tmp[j] = 0;
            else
            {
                int t0 = tmp[j-step*2-1] + LONG_DIST;
                int t = tmp[j-step*2+1] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-step-2] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-step-1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-step] + HV_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-step+1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-step+2] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j-1] + HV_DIST;
                if( t0 > t ) t0 = t;
                tmp[j] = t0;
            }
        }
    }

    // backward pass
    for( i = size.height - 1; i >= 0; i-- )
    {
        float* d = (float*)(dist + i*dststep);
        int* tmp = (int*)(temp + (i+BORDER)*step) + BORDER;
        
        for( j = size.width - 1; j >= 0; j-- )
        {
            int t0 = tmp[j];
            if( t0 > HV_DIST )
            {
                int t = tmp[j+step*2+1] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step*2-1] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step+2] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step+1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step] + HV_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step-1] + DIAG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+step-2] + LONG_DIST;
                if( t0 > t ) t0 = t;
                t = tmp[j+1] + HV_DIST;
                if( t0 > t ) t0 = t;
                tmp[j] = t0;
            }
            d[j] = (float)(t0 * scale);
        }
    }

    return CV_OK;
}


static CvStatus CV_STDCALL
icvDistanceTransformEx_5x5_C1R( const uchar* src, int srcstep, int* temp,
                int step, float* dist, int dststep, int* labels, int lstep,
                CvSize size, const float* metrics )
{
    const int BORDER = 2;
    
    int i, j;
    const int HV_DIST = CV_FLT_TO_FIX( metrics[0], ICV_DIST_SHIFT );
    const int DIAG_DIST = CV_FLT_TO_FIX( metrics[1], ICV_DIST_SHIFT );
    const int LONG_DIST = CV_FLT_TO_FIX( metrics[2], ICV_DIST_SHIFT );
    const float scale = 1.f/(1 << ICV_DIST_SHIFT);

    srcstep /= sizeof(src[0]);
    step /= sizeof(temp[0]);
    dststep /= sizeof(dist[0]);
    lstep /= sizeof(labels[0]);

    icvInitTopBottom( temp, step, size, BORDER );

    // forward pass
    for( i = 0; i < size.height; i++ )
    {
        const uchar* s = src + i*srcstep;
        int* tmp = (int*)(temp + (i+BORDER)*step) + BORDER;
        int* lls = (int*)(labels + i*lstep);

        for( j = 0; j < BORDER; j++ )
            tmp[-j-1] = tmp[size.width + j] = ICV_INIT_DIST0;
        
        for( j = 0; j < size.width; j++ )
        {
            if( !s[j] )
            {
                tmp[j] = 0;
                //assert( lls[j] != 0 );
            }
            else
            {
                int t0 = ICV_INIT_DIST0, t;
                int l0 = 0;

                t = tmp[j-step*2-1] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep*2-1];
                }
                t = tmp[j-step*2+1] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep*2+1];
                }
                t = tmp[j-step-2] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep-2];
                }
                t = tmp[j-step-1] + DIAG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep-1];
                }
                t = tmp[j-step] + HV_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep];
                }
                t = tmp[j-step+1] + DIAG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep+1];
                }
                t = tmp[j-step+2] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-lstep+2];
                }
                t = tmp[j-1] + HV_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j-1];
                }

                tmp[j] = t0;
                lls[j] = l0;
            }
        }
    }

    // backward pass
    for( i = size.height - 1; i >= 0; i-- )
    {
        float* d = (float*)(dist + i*dststep);
        int* tmp = (int*)(temp + (i+BORDER)*step) + BORDER;
        int* lls = (int*)(labels + i*lstep);
        
        for( j = size.width - 1; j >= 0; j-- )
        {
            int t0 = tmp[j];
            int l0 = lls[j];
            if( t0 > HV_DIST )
            {
                int t = tmp[j+step*2+1] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep*2+1];
                }
                t = tmp[j+step*2-1] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep*2-1];
                }
                t = tmp[j+step+2] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep+2];
                }
                t = tmp[j+step+1] + DIAG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep+1];
                }
                t = tmp[j+step] + HV_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep];
                }
                t = tmp[j+step-1] + DIAG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep-1];
                }
                t = tmp[j+step-2] + LONG_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+lstep-2];
                }
                t = tmp[j+1] + HV_DIST;
                if( t0 > t )
                {
                    t0 = t;
                    l0 = lls[j+1];
                }
                tmp[j] = t0;
                lls[j] = l0;
            }
            d[j] = (float)(t0 * scale);
        }
    }

    return CV_OK;
}


static CvStatus
icvGetDistanceTransformMask( int maskType, float *metrics )
{
    if( !metrics )
        return CV_NULLPTR_ERR;

    switch (maskType)
    {
    case 30:
        metrics[0] = 1.0f;
        metrics[1] = 1.0f;
        break;

    case 31:
        metrics[0] = 1.0f;
        metrics[1] = 2.0f;
        break;

    case 32:
        metrics[0] = 0.955f;
        metrics[1] = 1.3693f;
        break;

    case 50:
        metrics[0] = 1.0f;
        metrics[1] = 1.0f;
        metrics[2] = 2.0f;
        break;

    case 51:
        metrics[0] = 1.0f;
        metrics[1] = 2.0f;
        metrics[2] = 3.0f;
        break;

    case 52:
        metrics[0] = 1.0f;
        metrics[1] = 1.4f;
        metrics[2] = 2.1969f;
        break;
    default:
        return CV_BADRANGE_ERR;
    }

    return CV_OK;
}


/*********************************** IPP functions *********************************/

icvDistanceTransform_3x3_8u32f_C1R_t icvDistanceTransform_3x3_8u32f_C1R_p = 0;
icvDistanceTransform_5x5_8u32f_C1R_t icvDistanceTransform_5x5_8u32f_C1R_p = 0;

typedef CvStatus (CV_STDCALL * CvIPPDistTransFunc)( const uchar* src, int srcstep,
                                                    float* dst, int dststep,
                                                    CvSize size, const float* metrics );

/***********************************************************************************/

typedef CvStatus (CV_STDCALL * CvDistTransFunc)( const uchar* src, int srcstep,
                                                 int* temp, int tempstep,
                                                 float* dst, int dststep,
                                                 CvSize size, const float* metrics );

/* Wrapper function for distance transform group */
CV_IMPL void
cvDistTransform( const void* srcarr, void* dstarr,
                 int distType, int maskSize,
                 const float *mask,
                 void* labelsarr )
{
    CvMat* temp = 0;
    CvMat* src_copy = 0;
    CvMemStorage* st = 0;
    
    CV_FUNCNAME( "cvDistTransform" );

    __BEGIN__;

    float _mask[5];
    CvMat srcstub, *src = (CvMat*)srcarr;
    CvMat dststub, *dst = (CvMat*)dstarr;
    CvMat lstub, *labels = (CvMat*)labelsarr;
    CvSize size;
    CvIPPDistTransFunc ipp_func = 0;

    CV_CALL( src = cvGetMat( src, &srcstub ));
    CV_CALL( dst = cvGetMat( dst, &dststub ));

    if( !CV_IS_MASK_ARR( src ) || CV_MAT_TYPE( dst->type ) != CV_32FC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "source image must be 8uC1 and the distance map must be 32fC1" );

    if( !CV_ARE_SIZES_EQ( src, dst ))
        CV_ERROR( CV_StsUnmatchedSizes, "the source and the destination images must be of the same size" );

    if( maskSize != CV_DIST_MASK_3 && maskSize != CV_DIST_MASK_5 )
        CV_ERROR( CV_StsBadSize, "Mask size should be 3 or 5" );

    if( distType == CV_DIST_C || distType == CV_DIST_L1 )
        maskSize = !labels ? CV_DIST_MASK_3 : CV_DIST_MASK_5;
    else if( distType == CV_DIST_L2 && labels )
        maskSize = CV_DIST_MASK_5;

    if( labels )
    {
        CV_CALL( labels = cvGetMat( labels, &lstub ));
        if( CV_MAT_TYPE( labels->type ) != CV_32SC1 )
            CV_ERROR( CV_StsUnsupportedFormat, "the output array of labels must be 32sC1" );

        if( !CV_ARE_SIZES_EQ( labels, dst ))
            CV_ERROR( CV_StsUnmatchedSizes, "the array of labels has a different size" );

        if( maskSize == CV_DIST_MASK_3 )
            CV_ERROR( CV_StsNotImplemented,
            "3x3 mask can not be used for \"labeled\" distance transform. Use 5x5 mask" );
    }

    if( distType == CV_DIST_C || distType == CV_DIST_L1 || distType == CV_DIST_L2 )
    {
        icvGetDistanceTransformMask( (distType == CV_DIST_C ? 0 :
            distType == CV_DIST_L1 ? 1 : 2) + maskSize*10, _mask );
    }
    else if( distType == CV_DIST_USER )
    {
        if( !mask )
            CV_ERROR( CV_StsNullPtr, "" );

        memcpy( _mask, mask, (maskSize/2 + 1)*sizeof(float));
    }

    if( !labels )
        ipp_func = maskSize == CV_DIST_MASK_3 ? icvDistanceTransform_3x3_8u32f_C1R_p :
                                                icvDistanceTransform_5x5_8u32f_C1R_p;

    size = cvGetMatSize(src);

    if( ipp_func )
    {
        IPPI_CALL( ipp_func( src->data.ptr, src->step, dst->data.fl, dst->step, size, _mask ));
    }
    else
    {
        int border = maskSize == CV_DIST_MASK_3 ? 1 : 2;
        CV_CALL( temp = cvCreateMat( size.height + border*2, size.width + border*2, CV_32SC1 ));

        if( !labels )
        {
            CvDistTransFunc func = maskSize == CV_DIST_MASK_3 ?
                icvDistanceTransform_3x3_C1R :
                icvDistanceTransform_5x5_C1R;

            func( src->data.ptr, src->step, temp->data.i, temp->step,
                  dst->data.fl, dst->step, size, _mask );
        }
        else
        {
            CvSeq *contours = 0;
            int label;

            CV_CALL( st = cvCreateMemStorage() );
            CV_CALL( src_copy = cvCreateMat( size.height, size.width, src->type ));
            cvCmpS( src, 0, src_copy, CV_CMP_EQ );
            cvFindContours( src_copy, st, &contours, sizeof(CvContour),
                            CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE );
            cvZero( labels );
            for( label = 1; contours != 0; contours = contours->h_next, label++ )
                cvDrawContours( labels, contours, cvScalarAll(label), cvScalarAll(label), -255, -1, 8 );

            cvCopy( src, src_copy );
            cvRectangle( src_copy, cvPoint(0,0), cvPoint(size.width-1,size.height-1), cvScalarAll(255), 1, 8 );

            icvDistanceTransformEx_5x5_C1R( src_copy->data.ptr, src_copy->step, temp->data.i, temp->step,
                        dst->data.fl, dst->step, labels->data.i, labels->step, size, _mask );
        }
    }

    __END__;

    cvReleaseMat( &temp );
    cvReleaseMat( &src_copy );
    cvReleaseMemStorage( &st );
}

/* End of file. */
