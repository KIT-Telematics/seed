#include "omnetpp.h"

class SIM_API BinningAverageFilter : public cResultFilter
{
    protected:
        simtime_t binDuration;
        unsigned long binIndex;
        double accum;
    protected:
      virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, bool b)
      {
          receiveSignal(prev, t, (double)b);
      }

      virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, long l)
      {
          receiveSignal(prev, t, (double)l);
      }

      virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, unsigned long l)
      {
          receiveSignal(prev, t, (double)l);
      }

      virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, double d)
      {
          for (; (binIndex+1)*binDuration < t; binIndex++)
          {
              fire(this, binIndex*binDuration, accum/binDuration);
              accum = 0.0;
          }

          accum += d;
      }

      virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, const SimTime& v)
      {
          receiveSignal(prev, t, v.dbl());
      }

      virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, const char *s)
      {
          throw cRuntimeError("%s: Cannot convert const char * to double", getClassName());
      }

      virtual void receiveSignal(cResultFilter *prev, simtime_t_cref t, cObject *obj)
      {
          throw cRuntimeError("%s: Cannot convert cObject * to double", getClassName());
      }
    public:
        BinningAverageFilter() :
            binDuration(0.1),
            binIndex(0),
            accum(0.0)
        { }
};

Register_ResultFilter("binavg", BinningAverageFilter)
