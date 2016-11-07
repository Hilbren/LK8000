#ifndef FLARMCALCULATIONS_H
#define FLARMCALCULATIONS_H

#include <math.h>
#include <map>
#include "ClimbAverageCalculator.h"

class FlarmCalculations
{
public:
  FlarmCalculations(void);
  ~FlarmCalculations(void);
  double Average30s(long flarmId, double curTime, double curAltitude);
private:
  typedef std::map<long, ClimbAverageCalculator<30> > AverageCalculatorMap;
  AverageCalculatorMap averageCalculatorMap;
};

#endif
