#include "omnetpp.h"
#include "distribution.h"
#include "internal.h"

bool parseArray(
    std::vector<std::string>& args,
    std::vector<std::pair<double, std::string>>& res
)
{
    for (arg : args) {
        double number;
        std::string unit;
        if (!parseUnitNumber(arg, number, unit)) return false;
        res.push_back(std::make_pair(number, unit));
    }
    return true;
}

bool parseDistribtion(
    std::string expr,
    std::string unit,
    std::function<double(double)>& res
)
{
    Term term;
    std::string nv;
    std::vector<std::string> _args;
    if (!parseTerm(expr, term, nv, _args))
        return false;

    std::vector<std::pair<double, std::string>> args;

    if (term == FUNCTION)
    {
        if (!parseArray(_args, args)) return false;
    }
    else if (term == VARIABLE)
    {
        double number;
        std::string unit;
        if (!parseUnitNumber(nv, number, unit)) return false;
        args.push_back(std::make_pair(number, unit));
        nv = "interval";
    } else return false;

    if (nv == "interval")
    {
        if (args.size() != 1) return false;
        if (args[0].second != unit) return false;
        double interval = args[0].first;
        res = [interval](double rand) {
            return interval;
        };
    }
    else if (nv == "uniform")
    {
        if (args.size() != 2) return false;
        if (args[0].second != unit) return false;
        if (args[1].second != unit) return false;
        double lower = args[0].first;
        double upper = args[1].first;
        res = [lower, upper](double rand) {
            return lower + (upper - lower) * rand;
        };
    }
    else if (nv == "exp")
    {
        if (args.size() != 1) return false;
        if (args[0].second != unit) return false;
        double invlambda = args[0].first;
        res = [invlambda](double rand) {
            return -log(rand) * invlambda;
        };
    }
    else if (nv == "pareto")
    {
        if (args.size() != 2) return false;
        if (args[0].second != unit) return false;
        if (args[1].second != "") return false;
        double scale = args[0].first;
        double shape = args[1].first;
        res = [scale, shape](double rand) {
            return scale / pow(rand, 1/shape);
        };
    }
    else
        return false;
    return true;
}
