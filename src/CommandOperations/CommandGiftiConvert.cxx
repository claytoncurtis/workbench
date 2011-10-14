/*LICENSE_START*/ 
/* 
 *  Copyright 1995-2002 Washington University School of Medicine 
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

#include <fstream>
#include <ostream>
#include <iostream>

#include "CommandGiftiConvert.h"
#include "FileInformation.h"
#include "GiftiFile.h"
#include "ProgramParameters.h"

using namespace caret;

/**
 * Constructor.
 */
CommandGiftiConvert::CommandGiftiConvert()
: CommandOperation("-gifti-convert",
                         "Convert a GIFTI file to a different encoding")
{
    
}

/**
 * Destructor.
 */
CommandGiftiConvert::~CommandGiftiConvert()
{
    
}

/**
 * Execute the operation.
 * 
 * @param parameters
 *   Parameters for the operation.
 * @throws CommandException
 *   If the command failed.
 * @throws ProgramParametersException
 *   If there is an error in the parameters.
 */
void 
CommandGiftiConvert::executeOperation(ProgramParameters& parameters) throw (CommandException,
                                                               ProgramParametersException)
{
    const AString encodingName = parameters.nextString("GIFTI Encoding Name");
    const AString inputFileName = parameters.nextString("Input GIFTI File Name");
    const AString outputFileName = parameters.nextString("Output GIFTI File Name");
    
    bool isValidEncoding = false;
    GiftiEncodingEnum::Enum encoding = GiftiEncodingEnum::fromName(encodingName, &isValidEncoding);
    if (isValidEncoding == false) {
        throw CommandException("GIFTI Encoding is invalid.");
    }
    if (inputFileName.isEmpty()) {
        throw CommandException("Input GIFTI file name is empty.");
    }
    if (outputFileName.isEmpty()) {
        throw CommandException("Input GIFTI file name is empty.");
    }

    try {
        GiftiFile gf;
        gf.readFile(inputFileName);
        gf.setEncodingForWriting(encoding);
        gf.writeFile(outputFileName);
    }
    catch (GiftiException e) {
        throw CommandException(e);
    }
}



