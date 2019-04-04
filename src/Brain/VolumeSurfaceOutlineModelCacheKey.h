#ifndef __VOLUME_SURFACE_OUTLINE_MODEL_CACHE_KEY_H__
#define __VOLUME_SURFACE_OUTLINE_MODEL_CACHE_KEY_H__

/*LICENSE_START*/
/*
 *  Copyright (C) 2019 Washington University School of Medicine
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

#include "VolumeSliceViewPlaneEnum.h"

namespace caret {

    class VolumeSurfaceOutlineModelCacheKey : public CaretObject {
        
    public:
        VolumeSurfaceOutlineModelCacheKey(const VolumeSliceViewPlaneEnum::Enum slicePlane,
                                          const float sliceCoordinate);
        
        virtual ~VolumeSurfaceOutlineModelCacheKey();
        
        VolumeSurfaceOutlineModelCacheKey(const VolumeSurfaceOutlineModelCacheKey& obj);

        VolumeSurfaceOutlineModelCacheKey& operator=(const VolumeSurfaceOutlineModelCacheKey& obj);
        
        bool operator==(const VolumeSurfaceOutlineModelCacheKey& obj) const;
        
        bool operator<(const VolumeSurfaceOutlineModelCacheKey& obj) const;

        // ADD_NEW_METHODS_HERE

        virtual AString toString() const;
        
    private:
        void copyHelperVolumeSurfaceOutlineModelCacheKey(const VolumeSurfaceOutlineModelCacheKey& obj);

        VolumeSliceViewPlaneEnum::Enum m_slicePlane = VolumeSliceViewPlaneEnum::ALL;

        int32_t m_sliceCoordinateScaled = 100000;
        
        // ADD_NEW_MEMBERS_HERE

    };
    
#ifdef __VOLUME_SURFACE_OUTLINE_MODEL_CACHE_KEY_DECLARE__
    // <PLACE DECLARATIONS OF STATIC MEMBERS HERE>
#endif // __VOLUME_SURFACE_OUTLINE_MODEL_CACHE_KEY_DECLARE__

} // namespace
#endif  //__VOLUME_SURFACE_OUTLINE_MODEL_CACHE_KEY_H__
