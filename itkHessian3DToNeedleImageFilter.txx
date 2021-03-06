/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkHessian3DToNeedleImageFilter.txx,v $
  Language:  C++
  Date:      $Date: 2008-10-16 16:45:10 $
  Version:   $Revision: 1.7 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __itkHessian3DToNeedleImageFilter_txx
#define __itkHessian3DToNeedleImageFilter_txx

#include "itkHessian3DToNeedleImageFilter.h"
#include "itkImageRegionIterator.h"
#include "itkImageRegionConstIterator.h"
#include "vnl/vnl_math.h"

namespace itk
{

/**
 * Constructor
 */
template < typename TPixel >
Hessian3DToNeedleImageFilter< TPixel >
::Hessian3DToNeedleImageFilter()
{
  m_Alpha1 = 0.5;
  m_Alpha2 = 2.0;
  m_AngleThreshold = 0.0;
  m_MinimumLineMeasure = 5.0;
  m_Normal[0] = 0.0;
  m_Normal[1] = 0.0;
  m_Normal[2] = 1.0;

  m_PositiveContrast = false;

  // Hessian( Image ) = Jacobian( Gradient ( Image ) )  is symmetric
  m_SymmetricEigenValueFilter = EigenAnalysisFilterType::New();
  m_SymmetricEigenValueFilter->SetDimension( ImageDimension );
  m_SymmetricEigenValueFilter->OrderEigenValuesBy( 
      EigenAnalysisFilterType::OrderByValue );
  
}


template < typename TPixel >
void 
Hessian3DToNeedleImageFilter< TPixel >
::GenerateData()
{
  itkDebugMacro(<< "Hessian3DToNeedleImageFilter generating data ");

  m_SymmetricEigenValueFilter->SetInput( this->GetInput() );
  
  typename OutputImageType::Pointer output = this->GetOutput();

  typedef typename EigenAnalysisFilterType::OutputImageType
    EigenValueOutputImageType;

  m_SymmetricEigenValueFilter->Update();
  
  const typename EigenValueOutputImageType::ConstPointer eigenImage = 
                    m_SymmetricEigenValueFilter->GetOutput();

  typedef typename EigenAnalysisFilterType::EigenMatrixImageType
    EigenMatrixImageType;
  const typename EigenMatrixImageType::ConstPointer eigenMatrixImage = 
    m_SymmetricEigenValueFilter->GetEigenMatrixImage();
    

  // walk the region of eigen values and get the vesselness measure
  EigenValueArrayType eigenValue;
  ImageRegionConstIterator<EigenValueOutputImageType> it;
  it = ImageRegionConstIterator<EigenValueOutputImageType>(
      eigenImage, eigenImage->GetRequestedRegion());
  ImageRegionIterator<OutputImageType> oit;
  this->AllocateOutputs();
  oit = ImageRegionIterator<OutputImageType>(output,
                                             output->GetRequestedRegion());
  
  ImageRegionConstIterator<EigenMatrixImageType> eit;
  typename EigenMatrixImageType::RegionType eigenMatrixRegion;
  m_SymmetricEigenValueFilter->CallCopyOutputRegionToEigenMatrixImageRegion(eigenMatrixRegion, output->GetRequestedRegion());
  eit = ImageRegionConstIterator<EigenMatrixImageType>(eigenMatrixImage, eigenMatrixRegion);

  oit.GoToBegin();
  it.GoToBegin();
  eit.GoToBegin();

  double cosThreshold = vcl_cos((m_AngleThreshold / 180.0) * vnl_math::pi);

  while (!it.IsAtEnd())
    {
    // Get the eigen value
    eigenValue = it.Get();

    double sign;
    if (m_PositiveContrast)
      {
      sign = -1.0;
      }
    else
      {
      sign = 1.0;
      }

    // normalizeValue <= 0 for bright line structures
    double normalizeValue = vnl_math_min( -1.0 * eigenValue[1], -1.0 * eigenValue[0]);
    
    // Similarity measure to a line structure
    if( normalizeValue*sign < 0 )
      {
      double lineMeasure;
      if( eigenValue[2] <= 0 )
        {
        lineMeasure = 
          vcl_exp(-0.5 * vnl_math_sqr( eigenValue[2] / (m_Alpha1 * normalizeValue)));
        }
      else
        {
        lineMeasure = 
          vcl_exp(-0.5 * vnl_math_sqr( eigenValue[2] / (m_Alpha2 * normalizeValue)));
        }

      lineMeasure *= (-normalizeValue * sign);
      double ip = eit.Get()[0] * m_Normal[0] + eit.Get()[1] * m_Normal[1] + eit.Get()[2] * m_Normal[2];
      if (vnl_math_abs(ip) >= cosThreshold)
        {
          lineMeasure *= -normalizeValue * sign;
        }
      else
        {
          lineMeasure = 0.0;
        }
      if (lineMeasure > m_MinimumLineMeasure)
        {
        oit.Set( static_cast< OutputPixelType >(255) );
        }
      else
        {
        oit.Set( NumericTraits< OutputPixelType >::Zero );
        }
      }
    else
      {
      oit.Set( NumericTraits< OutputPixelType >::Zero );
      }

    ++it;
    ++oit;
    ++eit;
    }
    
}

template < typename TPixel >
void
Hessian3DToNeedleImageFilter< TPixel >
::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
  
  os << indent << "Alpha1: " << m_Alpha1 << std::endl;
  os << indent << "Alpha2: " << m_Alpha2 << std::endl;
}


} // end namespace itk
  
#endif
