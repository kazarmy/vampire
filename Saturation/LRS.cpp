/**
 * @file LRS.cpp
 * Implements class LRS.
 */

#include "../Lib/Environment.hpp"
#include "../Lib/Timer.hpp"
#include "../Lib/VirtualIterator.hpp"
#include "../Kernel/Clause.hpp"
#include "../Kernel/LiteralSelector.hpp"
#include "../Shell/Statistics.hpp"
#include "../Shell/Options.hpp"

#include "LRS.hpp"

namespace Saturation
{

using namespace Lib;
using namespace Kernel;
using namespace Shell;


/**
 * Return true if it is time to update age and weight
 * limits of the LRS strategy
 *
 * The time of the limit update is determined by a counter
 * of calls of this method.
 */
bool LRS::shouldUpdateLimits()
{
  CALL("LRS::shouldUpdateLimits");

  static unsigned cnt=0;
  cnt++;

  //when there are limits, we check more frequently so we don't skip too much inferences
  if(cnt==500 || ((getLimits()->weightLimited() || getLimits()->ageLimited()) && cnt>50 ) ) {
    cnt=0;
    return true;
  }
  return false;
}

/**
 * Resturn an estimate of the number of clauses that the saturation
 * algorithm will be able to activate in the remaining time
 */
long long LRS::estimatedReachableCount()
{
  CALL("LRS::estimatedReachableCount");

  long long processed=env.statistics->activeClauses;
  int currTime=env.timer->elapsedMilliseconds();
  long long timeSpent=currTime-_startTime;
  //the result is in miliseconds, as env.options->lrsFirstTimeCheck() is in percents.
  int firstCheck=env.options->lrsFirstTimeCheck()*env.options->timeLimitInDeciseconds();
//  int timeSpent=currTime;

  if(timeSpent<firstCheck ) {
    return -1;
  }

  long long timeLeft;
  if(env.options->simulatedTimeLimit()) {
    timeLeft=env.options->simulatedTimeLimit()*100 - currTime;
  } else {
    timeLeft=env.options->timeLimitInDeciseconds()*100 - currTime;
  }
  if(timeLeft<=0 || processed<=10) {
    //we end-up here even if there is no time timit (i.e. time limit is set to 0)
    return -1;
  }
  return (processed*timeLeft)/timeSpent;
}

SaturationResult LRS::doSaturation()
{
  CALL("LRS::doSaturation");

  bool complete=env.options->complete();

  for (;;) {
    newClausesToUnprocessed();

    while (! _unprocessed->isEmpty()) {
      Clause* c = _unprocessed->pop();
      ASS(!isRefutation(c));

      bool inPassive=false;
      if(forwardSimplify(c)) {
	backwardSimplify(c);
	inPassive=addToPassive(c);
      }
      if(inPassive) {
	ASS_EQ(c->store(), Clause::PASSIVE);
	_simplCont.add(c);
      } else {
	ASS_EQ(c->store(), Clause::UNPROCESSED);
	c->setStore(Clause::NONE);
      }

      newClausesToUnprocessed();

      if (env.timeLimitReached()) {
  	return SaturationResult(Statistics::TIME_LIMIT);
      }
      if(shouldUpdateLimits()) {
	long long estimatedReachable=estimatedReachableCount();
        if(estimatedReachable>=0) {
          _passive->updateLimits(estimatedReachable);
          if(complete) {
            Limits* lims=getLimits();
            complete=!lims->weightLimited() && !lims->ageLimited();
          }
        }
      }
    }
    onAllProcessed();
    if(!clausesFlushed()) {
      //there were some new clauses added, so let's process them
      continue;
    }

    if (_passive->isEmpty()) {
      return SaturationResult( complete ? Statistics::SATISFIABLE : Statistics::REFUTATION_NOT_FOUND );
    }

    Clause* c = _passive->popSelected();

    bool isActivated=activate(c);
    if(!isActivated) {
      handleUnsuccessfulActivation(c);
    }

    if(env.timeLimitReached()) {
      return SaturationResult(Statistics::TIME_LIMIT);
    }
  }
}

}
