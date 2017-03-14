#include <vts-libs/vts/glue.hpp>
#include <boost/algorithm/string/join.hpp>

using namespace vtslibs;
using namespace vtslibs::vts;
using namespace vtslibs::registry;

void print(TileSetGlues::list &lst)
{
    for (auto &&l : lst)
    {
        std::vector<std::string> s;
        for (auto &&g : l.glues)
        {
            std::string gn = boost::algorithm::join(g.id, ",");
            s.push_back(std::string() + "[" + gn + "]");
        }
        std::cout << l.tilesetId << " : "
                  << boost::algorithm::join(s, " , ") << std::endl;
    }
}

int main(int argc, const char *args[])
{
    
    TileSetGlues a("a");
    TileSetGlues b("b");
    TileSetGlues c("c");
    TileSetGlues d("d");
    
    c.glues.push_back(Glue({"a","b","c"}));
    d.glues.push_back(Glue({"a","d"}));
    c.glues.push_back(Glue({"a","c"}));
    //a.glues.push_back(Glue({"a"}));
    d.glues.push_back(Glue({"a","c","d"}));
    d.glues.push_back(Glue({"b","d"}));
    
    TileSetGlues::list lst;
    lst.push_back(a);
    lst.push_back(b);
    lst.push_back(c);
    lst.push_back(d);
    
    print(lst);
    lst = glueOrder(lst);
    std::cout << std::endl;
    print(lst);
    
    
    return 0;
}


