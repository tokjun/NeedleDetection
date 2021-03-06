/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkSymmetricEigenAnalysisImageFilterWithMatrix.txx,v $
  Language:  C++
  Date:      $Date: 2009-10-28 03:37:14 $
  Version:   $Revision: 1.34 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __itkSymmetricEigenAnalysisImageFilterWithMatrix_txx
#define __itkSymmetricEigenAnalysisImageFilterWithMatrix_txx

#include "itkSymmetricEigenAnalysisImageFilterWithMatrix.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "itkProgressReporter.h"

namespace itk
{

/**
 * Constructor
 */
template <class TInputImage, class TOutputImage  >
SymmetricEigenAnalysisImageFilterWithMatrix<TInputImage,TOutputImage>
::SymmetricEigenAnalysisImageFilterWithMatrix()
{
  this->SetNumberOfRequiredInputs( 1 );
  this->m_EigenMatrixImage = EigenMatrixImageType::New();

  this->InPlaceOff();
}

template <class TInputImage, class TOutputImage >
void 
SymmetricEigenAnalysisImageFilterWithMatrix<TInputImage,TOutputImage >
::CallCopyOutputRegionToEigenMatrixImageRegion(EigenMatrixImageType::RegionType &destRegion,
                                               const OutputImageRegionType &srcRegion)
{
  typedef ImageToImageFilterDetail::ImageRegionCopier<3,itkGetStaticConstMacro(OutputImageDimension)>
    OutputToEigenMatrixRegionCopierType;
  OutputToEigenMatrixRegionCopierType regionCopier;
  regionCopier(destRegion, srcRegion);
}



/** 
 * SymmetricEigenAnalysisImageFilterWithMatrix can produce an image which is a different resolution
 * than its input image.  As such, SymmetricEigenAnalysisImageFilterWithMatrix needs to provide an
 * implementation for GenerateOutputInformation() in order to inform
 * the pipeline execution model.  The original documentation of this
 * method is below.
 *
 * \sa ProcessObject::GenerateOutputInformaton() 
 */
template <class TInputImage, class TOutputImage >
void 
SymmetricEigenAnalysisImageFilterWithMatrix<TInputImage,TOutputImage >
::GenerateOutputInformation()
{
  // do not call the superclass' implementation of this method since
  // this filter allows the input the output to be of different dimensions
 
  // get pointers to the input and output
  typename Superclass::OutputImagePointer      outputPtr  = this->GetOutput();
  typename Superclass::InputImageConstPointer  inputPtr  = this->GetInput();

  if ( !outputPtr || !inputPtr || !this->m_EigenMatrixImage)
    {
    return;
    }

  // Set the output image largest possible region.  Use a RegionCopier
  // so that the input and output images can be different dimensions.
  OutputImageRegionType outputLargestPossibleRegion;
  this->CallCopyInputRegionToOutputRegion(outputLargestPossibleRegion,
                                          inputPtr->GetLargestPossibleRegion());
  outputPtr->SetLargestPossibleRegion( outputLargestPossibleRegion );

  EigenMatrixImageType::RegionType eigenMatrixRegion;
  this->CallCopyOutputRegionToEigenMatrixImageRegion(eigenMatrixRegion, inputPtr->GetLargestPossibleRegion());
  this->m_EigenMatrixImage->SetLargestPossibleRegion(eigenMatrixRegion);
  this->m_EigenMatrixImage->SetBufferedRegion(eigenMatrixRegion);
  this->m_EigenMatrixImage->Allocate();

  // Set the output spacing and origin
  const ImageBase<Superclass::InputImageDimension> *phyData;

  phyData
    = dynamic_cast<const ImageBase<Superclass::InputImageDimension>*>(this->GetInput());

  if (phyData)
    {
    // Copy what we can from the image from spacing and origin of the input
    // This logic needs to be augmented with logic that select which
    // dimensions to copy
    unsigned int i, j;
    const typename InputImageType::SpacingType&
      inputSpacing = inputPtr->GetSpacing();
    const typename InputImageType::PointType&
      inputOrigin = inputPtr->GetOrigin();
    const typename InputImageType::DirectionType&
      inputDirection = inputPtr->GetDirection();

    typename OutputImageType::SpacingType outputSpacing;
    typename OutputImageType::PointType outputOrigin;
    typename OutputImageType::DirectionType outputDirection;

    typename EigenMatrixImageType::SpacingType eigenMatrixImageSpacing;
    typename EigenMatrixImageType::PointType eigenMatrixImageOrigin;
    typename EigenMatrixImageType::DirectionType eigenMatrixImageDirection;

    // copy the input to the output and fill the rest of the
    // output with zeros.
    for (i=0; i < Superclass::InputImageDimension; ++i)
      {
      outputSpacing[i] = inputSpacing[i];
      outputOrigin[i] = inputOrigin[i];
      eigenMatrixImageSpacing[i] = inputSpacing[i];
      eigenMatrixImageOrigin[i] = inputOrigin[i];

      for (j=0; j < Superclass::OutputImageDimension; j++)
        {
        if (j < Superclass::InputImageDimension)
          {
          outputDirection[j][i] = inputDirection[j][i];
          eigenMatrixImageDirection[j][i] = inputDirection[j][i];
          }
        else
          {
          outputDirection[j][i] = 0.0;
          eigenMatrixImageDirection[j][i] = 0.0;
          }
        }
      }
    for (; i < Superclass::OutputImageDimension; ++i)
      {
      outputSpacing[i] = 1.0;
      outputOrigin[i] = 0.0;
      eigenMatrixImageSpacing[i] = 1.0;
      eigenMatrixImageOrigin[i] = 0.0;

      for (j=0; j < Superclass::OutputImageDimension; j++)
        {
        if (j == i)
          {
          outputDirection[j][i] = 1.0;
          eigenMatrixImageDirection[j][i] = 1.0;
          }
        else
          {
          outputDirection[j][i] = 0.0;
          eigenMatrixImageDirection[j][i] = 0.0;
          }
        }
      }

    // set the spacing and origin
    outputPtr->SetSpacing( outputSpacing );
    outputPtr->SetOrigin( outputOrigin );
    outputPtr->SetDirection( outputDirection );
    outputPtr->SetNumberOfComponentsPerPixel( // propagate vector length info
        inputPtr->GetNumberOfComponentsPerPixel());

    this->m_EigenMatrixImage->SetSpacing( eigenMatrixImageSpacing );
    this->m_EigenMatrixImage->SetOrigin( eigenMatrixImageOrigin );
    this->m_EigenMatrixImage->SetDirection( eigenMatrixImageDirection );
    this->m_EigenMatrixImage->SetNumberOfComponentsPerPixel( // propagate vector length info
        inputPtr->GetNumberOfComponentsPerPixel());
    }
  else
    {
    // pointer could not be cast back down
    itkExceptionMacro(<< "itk::SymmetricEigenAnalysisImageFilterWithMatrix::GenerateOutputInformation "
                      << "cannot cast input to "
                      << typeid(ImageBase<Superclass::InputImageDimension>*).name() );
    }
}


/**
 * ThreadedGenerateData Performs the pixel-wise addition
 */
template <class TInputImage, class TOutputImage >
void
SymmetricEigenAnalysisImageFilterWithMatrix<TInputImage,TOutputImage>
::ThreadedGenerateData( const OutputImageRegionType &outputRegionForThread,
                        ThreadIdType threadId)
{
  InputImagePointer  inputPtr = this->GetInput();
  OutputImagePointer outputPtr = this->GetOutput(0);
  
  
  // Define the portion of the input to walk for this thread, using
  // the CallCopyOutputRegionToInputRegion method allows for the input
  // and output images to be different dimensions
  InputImageRegionType inputRegionForThread;
  this->CallCopyOutputRegionToInputRegion(inputRegionForThread, outputRegionForThread);

  EigenMatrixImageType::RegionType eigenMatrixRegionForThread;
  CallCopyOutputRegionToEigenMatrixImageRegion(eigenMatrixRegionForThread, outputRegionForThread);

  // Define the iterators
  ImageRegionConstIterator<TInputImage>  inputIt(inputPtr, inputRegionForThread);
  ImageRegionIterator<TOutputImage> outputIt(outputPtr, outputRegionForThread);
  ImageRegionIterator<EigenMatrixImageType> eigenMatrixIt(this->m_EigenMatrixImage, eigenMatrixRegionForThread);

  ProgressReporter progress(this, threadId, outputRegionForThread.GetNumberOfPixels());

  inputIt.GoToBegin();
  outputIt.GoToBegin();
  eigenMatrixIt.GoToBegin();

  while( !inputIt.IsAtEnd() ) 
    {
    OutputImagePixelType eigenValue;
    EigenMatrixType eigenMatrix;
    //this->m_Analysis.ComputeEigenValues( inputIt.Get(), eigenValue );
    this->m_Analysis.ComputeEigenValuesAndVectors( inputIt.Get(), eigenValue, eigenMatrix );
    (eigenMatrixIt.Value())[0] = eigenMatrix[0][0];
    (eigenMatrixIt.Value())[1] = eigenMatrix[0][1];
    (eigenMatrixIt.Value())[2] = eigenMatrix[0][2];
    (eigenMatrixIt.Value())[3] = eigenMatrix[1][0];
    (eigenMatrixIt.Value())[4] = eigenMatrix[1][1];
    (eigenMatrixIt.Value())[5] = eigenMatrix[1][2];
    (eigenMatrixIt.Value())[6] = eigenMatrix[2][0];
    (eigenMatrixIt.Value())[7] = eigenMatrix[2][1];
    (eigenMatrixIt.Value())[8] = eigenMatrix[2][2];
    //outputIt.Set( eigenMatrix[0][2] );

    outputIt.Set( eigenValue );
    ++inputIt;
    ++outputIt;
    ++eigenMatrixIt;
    progress.CompletedPixel();  // potential exception thrown here
    }
}

} // end namespace itk

#endif
