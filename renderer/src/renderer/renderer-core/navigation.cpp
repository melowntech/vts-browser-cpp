#include "map.h"

namespace melown
{

void MapImpl::setMapConfig(const std::__cxx11::string &mapConfigPath)
{
    this->mapConfigPath = mapConfigPath;
}

void MapImpl::rotate(const vec3 &value)
{
    if (!mapConfig || !*mapConfig)
        return;
    
    vtslibs::registry::Position &pos = mapConfig->position;
    vec3 rot(value[0] * -0.2, value[1] * -0.1, 0);
    switch (mapConfig->srs.get
            (mapConfig->referenceFrame.model.navigationSrs).type)
    {
    case vtslibs::registry::Srs::Type::projected:
        break; // do nothing
    case vtslibs::registry::Srs::Type::geographic:
        rot(0) *= -1;
        break;
    default:
        throw std::invalid_argument("not implemented navigation srs type");
    }
    pos.orientation += vecToUblas<math::Point3>(rot);
    
    //LOG(info3) << "position: " << mapConfig->position.position;
    //LOG(info3) << "rotation: " << mapConfig->position.orientation;
}

void MapImpl::pan(const vec3 &value)
{
    if (!mapConfig || !*mapConfig)
        return;
    
    vtslibs::registry::Position &pos = mapConfig->position;
    switch (mapConfig->srs.get
            (mapConfig->referenceFrame.model.navigationSrs).type)
    {
    case vtslibs::registry::Srs::Type::projected:
    {
        mat3 rot = upperLeftSubMatrix
                (rotationMatrix(2, degToRad(pos.orientation(0))));
        vec3 move = vec3(-value[0], value[1], 0);
        move = rot * move * (pos.verticalExtent / 800);
        pos.position += vecToUblas<math::Point3>(move);
    } break;
    case vtslibs::registry::Srs::Type::geographic:
    {
        mat3 rot = upperLeftSubMatrix
                (rotationMatrix(2, degToRad(-pos.orientation(0))));
        vec3 move = vec3(-value[0], value[1], 0);
        move = rot * move * (pos.verticalExtent / 800);
        vec3 p = vecFromUblas<vec3>(pos.position);
        p = mapConfig->convertor->navGeodesicDirect(p, 90, move(0));
        p = mapConfig->convertor->navGeodesicDirect(p, 0, move(1));
        pos.position = vecToUblas<math::Point3>(p);
        pos.position(0) = modulo(pos.position(0) + 180, 360) - 180;
        // todo - temporarily clamp
        pos.position(1) = std::min(std::max(pos.position(1), -80.0), 80.0);
    } break;
    default:
        throw std::invalid_argument("not implemented navigation srs type");
    }
    pos.verticalExtent *= pow(1.001, -value[2]);
    
    panAdjustZ(vec3to2(vecFromUblas<vec3>(pos.position)));
    
    //LOG(info3) << "position: " << mapConfig->position.position;
    //LOG(info3) << "rotation: " << mapConfig->position.orientation;
}

} // namespace melown
