/*LICENSE_START*/
/* 
 *  Copyright 1995-2011 Washington University School of Medicine
 *
 *  http://brainmap.wustl.edu
 *
 *  This file is part of CARET.
 *
 *  CARET is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  CARET is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CARET; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "NiftiTest.h"
using namespace caret;

NiftiTest::NiftiTest(const AString &identifier) : TestInterface(identifier)
{
}

void NiftiTest::execute()
{

}

NiftiHeaderTest::NiftiHeaderTest(const AString &identifier) : TestInterface(identifier)
{
}

void NiftiHeaderTest::execute()
{
    Nifti1Header n1header;
    Nifti2Header n2header;
    readNifti1Header("../../../wb_files/nifti/fcMRI1_nonlin_Subcortical_Smoothed_s6.nii", n1header);
    readNifti2Header("../../../wb_files/nifti/test.cii", n2header);
    writeNifti1Header("../../../wb_files/nifti/n1.nii",n1header);
    writeNifti2Header("../../../wb_files/nifti/n2.cii",n2header);
    Nifti1Header n1headercomp;
    Nifti2Header n2headercomp;
    readNifti1Header("../../../wb_files/nifti/n1.nii", n1headercomp);
    readNifti2Header("../../../wb_files/nifti/n2.cii", n2headercomp);

    nifti_1_header n1struct;
    nifti_1_header n1struct2;
    n1header.getHeaderStruct(n1struct);
    n1headercomp.getHeaderStruct(n1struct2);
    if(memcmp(&n1struct,&n1struct2,sizeof(n1struct)))
    {
        //report error
        AString message;
        message = AString("Nifti1 Header Reader/Writer error, files do not match.");
        setFailed(message);
    }
    else
    {
        std::cout << "Nifti1 Header Reader/Writer passes basic tests" << std::endl;
    }

    nifti_2_header n2struct;
    nifti_2_header n2struct2;
    n2header.getHeaderStruct(n2struct);
    n2header.getHeaderStruct(n2struct2);
    if(memcmp(&n2struct,&n2struct2,sizeof(n2struct)))
    {
        //report error
        AString message;
        message = AString("Nifti2 Header Reader/Writer error, files do not match.");
        setFailed(message);
    }
    else
    {
        std::cout << "Nifti2 Header Reader/Writer passes basic tests" << std::endl;
    }
}





void NiftiHeaderTest::readNifti1Header(AString filename, Nifti1Header &header)
{
    NiftiHeaderIO *headerIO = new NiftiHeaderIO(filename);
    //test version check
    if(headerIO->getNiftiVersion() == 1)
    {
        headerIO->getHeader(header);
    }
    else
    {
        //report error
        AString message;
        message = AString("Incorrect Nifti version detected, expected version 1, got ") + AString::number(headerIO->getNiftiVersion()) + AString(".\n");
        setFailed(message);
    }
}

void NiftiHeaderTest::readNifti2Header(AString filename, Nifti2Header &header)
{
    NiftiHeaderIO *headerIO = new NiftiHeaderIO(filename);
    if(headerIO->getNiftiVersion() == 2)
    {
        headerIO->getHeader(header);
    }
    else
    {
        //report error
        AString message;
        message = AString("Incorrect Nifti version detected, expected version 2, got ") + AString::number(headerIO->getNiftiVersion()) + AString(".\n");
        setFailed(message);
    }
}

void NiftiHeaderTest::writeNifti1Header(AString filename, Nifti1Header &header)
{
    NiftiHeaderIO headerIO;
    headerIO.setHeader(header);

    //test version check
    if(headerIO.getNiftiVersion() == 1)
    {
        headerIO.writeFile(filename);
    }
    else
    {
        //report error
        AString message;
        message = AString("Incorrect Nifti version detected, expected version 1, got ") + AString::number(headerIO.getNiftiVersion()) + AString(".\n");
        setFailed(message);
    }
}


void NiftiHeaderTest::writeNifti2Header(AString filename, Nifti2Header &header)
{

    NiftiHeaderIO headerIO;
    headerIO.setHeader(header);
    if(headerIO.getNiftiVersion() == 2)
    {
        headerIO.writeFile(filename);
    }
    else
    {
        //report error
        AString message;
        message = AString("Incorrect Nifti version detected, expected version 2, got ") + AString::number(headerIO.getNiftiVersion()) + AString(".\n");
        setFailed(message);
    }
}



