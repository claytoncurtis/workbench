#ifndef __CZI_IMAGE_MASKING_FILE_H__
#define __CZI_IMAGE_MASKING_FILE_H__

/*LICENSE_START*/
/*
 *  Copyright (C) 2022 Washington University School of Medicine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*LICENSE_END*/



#include <memory>

#include "CaretObject.h"
#include "Vector3D.h"

namespace caret {
    class Matrix4x4;
    class VolumeFile;
    
    class CziImageMaskingFile : public CaretObject {
        
    public:
        enum class Status {
            INVALID,
            UNREAD,
            VALID
        };
        
        CziImageMaskingFile(const AString& filename);
        
        virtual ~CziImageMaskingFile();
        
        CziImageMaskingFile(const CziImageMaskingFile& obj) = delete;

        CziImageMaskingFile& operator=(const CziImageMaskingFile& obj) = delete;

        float getMaskingValue(const Vector3D& planeXYZ,
                              const float defaultValue) const;
        
        Status getStatus() const;
        
        AString getFilename() const;
        
        // ADD_NEW_METHODS_HERE

        virtual AString toString() const;
        
    private:
        void copyHelperCziImageMaskingFile(const CziImageMaskingFile& obj);

        void load(const AString& filename) const;
        
        void getNonLinearOffsetToMillimeters(const Vector3D& planeXYZ,
                                             Vector3D& offsetXyzOut);

        const AString m_filename;
        
        mutable std::unique_ptr<VolumeFile> m_niftiFile;
        
        mutable std::unique_ptr<Matrix4x4> m_sformMatrix;
        
        mutable std::unique_ptr<Matrix4x4> m_inverseSformMatrix;
        
        mutable Status m_status = Status::UNREAD;
                        
        static constexpr bool m_debugFlag = false;
        
        // ADD_NEW_MEMBERS_HERE

    };
    
#ifdef __CZI_IMAGE_MASKING_FILE_H_DECLARE__
    // <PLACE DECLARATIONS OF STATIC MEMBERS HERE>
#endif // __CZI_IMAGE_MASKING_FILE_H_DECLARE__

} // namespace
#endif  //__CZI_IMAGE_MASKING_FILE_H__
