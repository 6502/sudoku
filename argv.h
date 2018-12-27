#if !defined(PARMS_H_INCLUDED)
#define PARMS_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <vector>

struct Parm {
    static std::map<std::string, Parm *>& parms() {
        static std::map<std::string, Parm *> p;
        return p;
    }

    std::string name;
    std::string description;
    std::string value;

    Parm(const std::string& name, const std::string description, const std::string& value)
        : name(name), description(description), value(value)
    {
        parms()[name] = this;
    }

    virtual void parse(const char *v) = 0;
};

template<typename T>
void parse(T& x, const char *s);

template<typename T>
struct TParm : Parm {
    T* x;
    TParm(T *x, const std::string& name, const std::string& description, const std::string& value)
        : Parm{name, description, value}, x{x}
    {
        ::parse(*x, value.c_str());
    }
    virtual void parse(const char *v) { ::parse(*x, v); }
};

template<> void parse(int& x, const char *s) { x = atoi(s); }
template<> void parse(float& x, const char *s) { x = atof(s); }
template<> void parse(double& x, const char *s) { x = atof(s); }
template<> void parse(std::string& x, const char *s) { x = s; }

inline void parse_argv(const char *program, int argc, const char *argv[]) {
    auto help = [&](){
        fprintf(stderr, "%s - Valid parameters are:\n", program);
        for (auto& i : Parm::parms()) {
            fprintf(stderr, "  --%s : %s%s\n",
                    i.first.c_str(),
                    i.second->description.c_str(),
                    i.second->value.size() == 0 ? "" : (" [default " + i.second->value + "]").c_str());
        }
        exit(1);
    };
    for (int i=1; i<argc; i++) {
        if (i<argc-1 && argv[i][0] == '-' && argv[i][1] == '-') {
            auto it = Parm::parms().find(argv[i]+2);
            if (it == Parm::parms().end()) {
                fprintf(stderr, "Unknown parameter '%s'\n", argv[i]);
                help();
            }
            it->second->parse(argv[++i]);
        } else {
            fprintf(stderr, "Invalid syntax '%s'\n", argv[i]);
            help();
        }
    }
}

#define PARM(type, n, descr, value)              \
    type n; new TParm<type>(&n, #n, descr, value)

#endif
