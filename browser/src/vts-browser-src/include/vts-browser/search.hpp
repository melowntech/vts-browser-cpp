#ifndef SEARCH_H_gtvuigshefh
#define SEARCH_H_gtvuigshefh

#include <string>
#include <vector>
#include <memory>

#include "foundation.hpp"

namespace vts
{

class VTS_API SearchItem
{
public:
    SearchItem();
    
    std::string title;
    std::string region;
    std::string type;

    double position[3]; // navigation srs
    double radius; // physical srs length
    double distance; // physical srs length
};

class VTS_API SearchTask
{
public:
    SearchTask(const std::string &query, const double point[3]);
    virtual ~SearchTask();

    std::vector<SearchItem> results;

    const std::string query;
    const double position[3];

    bool done;
    
    std::shared_ptr<class SearchTaskImpl> impl;
};

} // namespace vts

#endif
