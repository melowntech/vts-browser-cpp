#include <jsoncpp/json.hpp>

#include "map.hpp"

namespace vts
{

namespace
{

std::string generateSearchUrl(MapImpl *impl, const std::string &query)
{
    std::string url = impl->options.searchUrl;
    static const std::string rep = "{query}";
    auto s = url.find(rep);
    if (s != std::string::npos)
        url.replace(s, rep.length(), query);
    return url;
}

void latlonToNav(MapImpl *map, double point[3])
{
    (void)map;
    (void)point;
    // todo
}

// both points are in navigation srs
double distance(MapImpl *map, const double a[], const double b[],
                double def = std::numeric_limits<double>::quiet_NaN())
{
    for (int i = 0; i < 3; i++)
        if (a[i] != a[i] || b[i] != b[i])
            return def;
    vec3 va(a[0], a[1], a[2]);
    vec3 vb(b[0], b[1], b[2]);
    switch (map->mapConfig->srs.get(
                map->mapConfig->referenceFrame.model.navigationSrs).type)
    {
    case vtslibs::registry::Srs::Type::cartesian:
    case vtslibs::registry::Srs::Type::projected:
        return length(vec3(vb - va));
    case vtslibs::registry::Srs::Type::geographic:
        return map->mapConfig->convertor->navGeodesicDistance(va, vb);
    }
    return def;
}

void filterSearchResults(MapImpl *, const std::shared_ptr<SearchTask> &task)
{
    // update some fields
    for (SearchItem &it : task->results)
    {
        // title
        {
            auto s = it.displayName.find(",");
            if (s != std::string::npos)
                it.title = it.displayName.substr(0, s);
            else
                it.title = it.displayName;
        }
        // region
        {
            if (!it.state.empty())
                it.region = it.state;
            else if (!it.stateDistrict.empty())
                it.region = it.stateDistrict;
            else
                it.region = it.county;
            auto s = it.region.find(" - ");
            if (s != std::string::npos)
                it.region = it.region.substr(0, s);
        }
        // street name queries
        if (it.title == it.houseNumber && !it.road.empty())
            it.title = it.road;
    }
    
    // filter results, that are close to each other
    
    // reshake hits by distance
}

double vtod(Json::Value &v)
{
    if (v.type() == Json::ValueType::realValue)
        return v.asDouble();
    double r = std::numeric_limits<double>::quiet_NaN();
    sscanf(v.asString().c_str(), "%lf", &r);
    return r;
}

void parseSearchResults(MapImpl *map, const std::shared_ptr<SearchTask> &task)
{
    assert(!task->done);
    try
    {
        Json::Value root;
        {
            Json::Reader r;
            detail::Wrapper w(task->impl->data);
            if (!r.parse(w, root, false))
                LOGTHROW(err2, std::runtime_error)
                        << "failed to parse search result json, url: '"
                        << task->impl->fetch->name << "', error: '"
                        << r.getFormattedErrorMessages() << "'";
        }
        for (Json::Value &it : root)
        {
            SearchItem t;
            t.displayName = it["display_name"].asString();
            t.type = it["type"].asString();
            Json::Value addr = it["address"];
            if (!addr.empty())
            {
                t.houseNumber = addr["house_number"].asString();
                t.road = addr["road"].asString();
                t.city = addr["city"].asString();
                t.county = addr["county"].asString();
                t.state = addr["state"].asString();
                t.stateDistrict = addr["state_district"].asString();
                t.country = addr["country"].asString();
                t.countryCode = addr["country_code"].asString();
            }
            t.position[0] = vtod(it["lon"]);
            t.position[1] = vtod(it["lat"]);
            t.position[2] = 0;
            latlonToNav(map, t.position);
            t.distance = distance(map, task->position, t.position);
            Json::Value bj = it["boundingbox"];
            if (bj.size() == 4)
            {
                double r[4];
                int i = 0;
                for (auto it : bj)
                    r[i++] = vtod(it);
                double bbs[4][3] = {
                    { r[2], r[0], 0 },
                    { r[2], r[1], 0 },
                    { r[3], r[0], 0 },
                    { r[3], r[1], 0 }
                };
                t.radius = 0;
                for (int i = 0; i < 4; i++)
                {
                    latlonToNav(map, bbs[i]);
                    t.radius = std::max(t.radius,
                                        distance(map, t.position, bbs[i]));
                }
            }
            task->results.push_back(t);
        }
        if (map->options.searchResultsFilter)
            filterSearchResults(map, task);
    }
    catch (const Json::Exception &e)
    {
        LOG(err3) << "failed to process search results, url: '"
                  << task->impl->fetch->name << "', query: '"
                  << task->query << "', error: '"
                  << e.what() << "'";
    }
    catch (...)
    {
        LOG(err3) << "failed to process search results, url: '"
                  << task->impl->fetch->name << "', query: '"
                  << task->query << "'";
    }
}

} // namespace

void SearchTaskImpl::load()
{
    data = std::move(fetch->contentData);
}

SearchItem::SearchItem() :
    position{ std::numeric_limits<double>::quiet_NaN(),
              std::numeric_limits<double>::quiet_NaN(),
              std::numeric_limits<double>::quiet_NaN()},
    radius(std::numeric_limits<double>::quiet_NaN()),
    distance(std::numeric_limits<double>::quiet_NaN()),
    importance(-1)
{}

SearchTask::SearchTask(const std::string &query, const double point[3]) :
    query(query), position{point[0], point[1], point[2]}, done(false)
{}

SearchTask::~SearchTask()
{}

std::shared_ptr<SearchTask> MapImpl::search(const std::string &query,
                                            const double point[3])
{
    auto t = std::make_shared<SearchTask>(query, point);
    t->impl = getSearchImpl(generateSearchUrl(this, query));
    resources.searchTasks.push_back(t);
    return t;
}

void MapImpl::updateSearch()
{
    auto it = resources.searchTasks.begin();
    while (it != resources.searchTasks.end())
    {
        std::shared_ptr<SearchTask> t = it->lock();
        if (t)
        {
            switch (getResourceValidity(t->impl))
            {
            case Validity::Indeterminate:
                touchResource(t->impl);
                it++;
                continue;
            case Validity::Invalid:
                break;
            case Validity::Valid:
                parseSearchResults(this, t);
            }
            t->done = true;
        }
        it = resources.searchTasks.erase(it);
    }
}

} // namespace vts
