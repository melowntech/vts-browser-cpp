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

void processSearchResults(MapImpl *, const std::shared_ptr<SearchTask> &task)
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
                        << task->impl->fetch->name << "', error(s): '"
                        << r.getFormattedErrorMessages() << "'";
        }
        for (auto it : root)
        {
            SearchItem t;
            t.title = it["display_name"].asString();
            task->results.push_back(t);
        }
    }
    catch (const std::runtime_error &)
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

SearchItem::SearchItem() : position{0,0,0}, radius(0), distance(0)
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
                processSearchResults(this, t);
            }
            t->done = true;
        }
        it = resources.searchTasks.erase(it);
    }
}

} // namespace vts
