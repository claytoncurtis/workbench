
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

#define __SPEC_FILE_GROUP_DECLARE__
#include "SpecFileDataFileTypeGroup.h"
#undef __SPEC_FILE_GROUP_DECLARE__

#include "CaretAssert.h"

using namespace caret;

/**
 * \class SpecFileDataFileTypeGroup
 * \brief Groups files of the same DataFileType.
 *
 * Groups files of the same data type in a SpecFile.
 */

/**
 * Constructor.
 * @param dataFileType
 *     Type of data file.
 */
SpecFileDataFileTypeGroup::SpecFileDataFileTypeGroup(const DataFileTypeEnum::Enum dataFileType)
: CaretObject()
{
    this->dataFileType = dataFileType;
}

/**
 * Destructor.
 */
SpecFileDataFileTypeGroup::~SpecFileDataFileTypeGroup()
{
    this->removeAllFileInformation();
}

/**
 * @return The data file type for this group.
 */
DataFileTypeEnum::Enum 
SpecFileDataFileTypeGroup::getDataFileType() const
{
    return this->dataFileType;
}

/**
 * @return The number of files.
 */
int32_t 
SpecFileDataFileTypeGroup::getNumberOfFiles() const
{
    return this->files.size();
}

/**
 * @return The number of files selected.
 */
int32_t 
SpecFileDataFileTypeGroup::getNumberOfFilesSelected() const
{
    int count = 0;
    
    for (std::vector<SpecFileDataFile*>::const_iterator iter = this->files.begin();
         iter != this->files.end();
         iter++) {
        SpecFileDataFile* file = *iter;
        if (file->isSelected()) {
            count++;
        }
    }
    return count;
}

/**
 * Add a file.
 * @param fileInformation
 *    New file information.
 */
void 
SpecFileDataFileTypeGroup::addFileInformation(SpecFileDataFile* fileInformation)
{
    CaretAssert(fileInformation);
    this->files.push_back(fileInformation);
}

/**
 * Remove file at given index.
 * @param fileIndex
 *    Index of file for removal.
 */
void 
SpecFileDataFileTypeGroup::removeFileInformation(const int32_t fileIndex)
{
    CaretAssertVectorIndex(this->files, 
                           fileIndex);
    delete this->files[fileIndex];
    this->files.erase(this->files.begin() + fileIndex);
}

/**
 * Remove all files.
 */
void 
SpecFileDataFileTypeGroup::removeAllFileInformation()
{
    for (std::vector<SpecFileDataFile*>::iterator iter = this->files.begin();
         iter != this->files.end();
         iter++) {
        delete *iter;
    }
    this->files.clear();
}

/**
 * Get information for a file.
 * @param fileIndex
 *    Index of file for which information is requested.
 * @return
 *    Information for file.
 */
SpecFileDataFile* 
SpecFileDataFileTypeGroup::getFileInformation(const int32_t fileIndex)
{
    CaretAssertVectorIndex(this->files, 
                           fileIndex);
    return this->files[fileIndex];
}

/**
 * Set the selection status of all files.
 * @param selectionStatus
 *   New selection status for all files.
 */
void 
SpecFileDataFileTypeGroup::setAllFilesSelected(bool selectionStatus)
{
    for (std::vector<SpecFileDataFile*>::iterator iter = this->files.begin();
         iter != this->files.end();
         iter++) {
        SpecFileDataFile* file = *iter;
        file->setSelected(selectionStatus);
    }
}

/**
 * Get a description of this object's content.
 * @return String describing this object's content.
 */
AString 
SpecFileDataFileTypeGroup::toString() const
{
    AString info = "DataFileType=" + DataFileTypeEnum::toName(this->dataFileType) + "\n";
    for (std::vector<SpecFileDataFile*>::const_iterator iter = this->files.begin();
         iter != this->files.end();
         iter++) {
        SpecFileDataFile* file = *iter;
        info += ("    " + file->toString() + "\n");

    }
    
    return info;
}    
