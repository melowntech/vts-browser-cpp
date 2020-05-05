/**
 * Copyright (c) 2020 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef EXPLICITTRAVERSAL_HPP_1z6uoi
#define EXPLICITTRAVERSAL_HPP_1z6uoi

#include <string>
#include <memory>

#include "foundation.hpp"

namespace vts
{

class MapImpl;
class CameraCredits;
class CameraDraws;
class Map;
class CameraImpl;
class TraverseNode;

struct VTS_API ExplicitTraversalIterator
{
    ExplicitTraversalIterator();
    explicit ExplicitTraversalIterator(TraverseNode *node);

    bool isMetaReady() const;
    bool loadMeta();

    uint32 childrenCount() const;
    ExplicitTraversalIterator child(uint32 index) const;
    ExplicitTraversalIterator parent() const;

    double texelSize() const;
    void alignedBox(double minMax[6]) const;
    bool orientedBox(double matrix[16], double minMax[6]) const;
    bool surrogate(double coords[3]) const;

    bool isDrawsReady() const;
    bool loadDraws();

    CameraCredits credits();
    CameraDraws draws();

    //friend bool operator < (const ExplicitTraversalIterator &a, const ExplicitTraversalIterator &b);
    friend bool operator == (const ExplicitTraversalIterator &a, const ExplicitTraversalIterator &b);
    friend bool operator != (const ExplicitTraversalIterator &a, const ExplicitTraversalIterator &b);

private:
    TraverseNode *impl;
};

class VTS_API EplicitTraversal : private Immovable
{
public:
    explicit EplicitTraversal(MapImpl *map);

    // invalidates all iterators
    void renderUpdate();

    uint32 layersCount() const;
    std::string layerName(uint32 layerIndex) const;
    bool isLayerGeodata(uint32 layerIndex) const;
    ExplicitTraversalIterator traverseRoot(uint32 layerIndex);

    Map *map() const;

private:
    std::shared_ptr<CameraImpl> impl;
    friend Map;
};

} // namespace vts

#endif
