#ifndef FETCHER_H_shvgcfjs
#define FETCHER_H_shvgcfjs

#include <memory>
#include <string>

#include <renderer/fetcher.h>

class Fetcher : public melown::Fetcher
{
public:
    static Fetcher *create();

    virtual void tick() = 0;

    std::string username;
    std::string password;
};

#endif
