#include "Inverters.hpp"
#include "Debug/Tracer.hpp"

namespace Kernel {
namespace Rebalancing {
namespace Inverters {
#define DEBUG(...) //DBG(__VA_ARGS__)

template<class A> void __ignoreWarnUnusedLocalTypedefHack() {}

#define CASE_INVERT(sort, fun, expr)                                           \
  case NumTraits<sort>::fun##I: {                                              \
    using number = NumTraits<sort>;                                            \
    __ignoreWarnUnusedLocalTypedefHack<number>();                              \
    return expr;                                                               \
  }

#define CASE_INVERT_INT(fun, expr) CASE_INVERT(IntegerConstantType, fun, expr)

#define CASE_INVERT_FRAC(fun, expr)                                            \
  CASE_INVERT(RealConstantType, fun, expr) \
  CASE_INVERT(RationalConstantType, fun, expr)

bool canInvertMulInt(const InversionContext &ctxt);
TermList doInvertMulInt(const InversionContext &ctxt);
template <class number> bool nonZero(const TermList &t);

bool NumberTheoryInverter::canInvertTop(const InversionContext &ctxt) {
  CALL("NumberTheoryInverter::canInvertTop")
  auto &t = ctxt.topTerm();
  auto fun = t.functor();
  auto sym = env.signature->getFunction(fun);

  DEBUG("canInvert ", ctxt.topTerm().toString(), "@", ctxt.topIdx())
  if (theory->isInterpretedFunction(fun)) {
    auto inter = theory->interpretFunction(fun);
    switch (inter) {
      CASE_INVERT_FRAC(add, true)
      CASE_INVERT_FRAC(minus, true)
      CASE_INVERT_FRAC(mul, nonZero<number>(t[1 - ctxt.topIdx()]))
      CASE_INVERT_INT(mul, canInvertMulInt(ctxt))
      CASE_INVERT_INT(add, true)
      CASE_INVERT_INT(minus, true)
    case Theory::Interpretation::ARRAY_STORE:
      /* store(t, i, x) = s ==> x = select(s, i) */
      return ctxt.topIdx() == 2;
      default:;
    }
    // DBG("WARNING: unknown interpreted function: ", t.toString())
    return false;
  } else if (sym->termAlgebraCons()) { 
    return true;
  } else { /* cannot invert uninterpreted functions */
    DEBUG("no")
    return false;
  }
}

#define CASE_DO_INVERT(sort, fun, expr)                                        \
  case NumTraits<sort>::fun##I: {                                              \
    using number = NumTraits<sort>;                                            \
    __ignoreWarnUnusedLocalTypedefHack<number>();                              \
    return expr;                                                               \
  }

#define CASE_DO_INVERT_FRAC(fun, expr)                                         \
  CASE_DO_INVERT(RealConstantType, fun, expr)                                  \
  CASE_DO_INVERT(RationalConstantType, fun, expr)

#define CASE_DO_INVERT_ALL(fun, expr)                                          \
  CASE_DO_INVERT_INT(fun, expr)                                                \
  CASE_DO_INVERT_FRAC(fun, expr)

#define CASE_DO_INVERT_INT(fun, expr)                                          \
  CASE_DO_INVERT(IntegerConstantType, fun, expr)

TermList NumberTheoryInverter::invertTop(const InversionContext &ctxt) {
  CALL("NumberTheoryInverter::invertTop")
  ASS(canInvertTop(ctxt))
  // DBG("inverting: ", ctxt.topTerm().toString())
  auto &t = ctxt.topTerm();
  auto index = ctxt.topIdx();
  auto toWrap = ctxt.toWrap();
  auto fun = t.functor();
  DEBUG("inverting ", ctxt.topTerm().toString())
  if(theory->isInterpretedFunction(fun)) {
    switch (theory->interpretFunction(fun)) {

      CASE_DO_INVERT_ALL(add, number::add(toWrap, number::minus(t[1 - index])))
      CASE_DO_INVERT_ALL(minus, number::minus(toWrap))

      CASE_DO_INVERT_FRAC(
          mul, number::mul(toWrap, number::div(number::one(), t[1 - index])))
      CASE_DO_INVERT_INT(mul, doInvertMulInt(ctxt))

      case Theory::Interpretation::ARRAY_STORE: 
      {
        ASS(ctxt.topIdx() == 2)
        /*              store(t, i, x) = s ==> x = select(s, i) */
        /* auto toWrap:                  ^                      */
        /* auto& t:     ^^^^^^^^^^^^^^                          */
        auto& store = *env.signature->getFunction(t.functor())->fnType();
        auto select = env.signature->getInterpretingSymbol(
            Theory::Interpretation::ARRAY_SELECT, 
            OperatorType::getFunctionType({ store.arg(0), store.arg(1) }, store.arg(2)));
        return TermList(Term::create2(select, toWrap, *t.nthArgument(1)));
      }
    default:
      ASSERTION_VIOLATION;
    }
  } else {
    // must be a term algebra sort
    auto sym = env.signature->getFunction(fun);
    ASS_REP(sym->termAlgebraCons(), sym);
    auto ctor = env.signature->getTermAlgebraConstructor(fun);
    auto dtor = ctor->destructorFunctor(index);
    return TermList(Term::create1(dtor, toWrap));
  }
};

bool tryInvertMulInt(const InversionContext &ctxt, TermList &out) {
  CALL("tryInvertMulInt(..)")
  using number = NumTraits<IntegerConstantType>;

  auto a_ = ctxt.topTerm()[1 - ctxt.topIdx()];
  IntegerConstantType a;
  if ( theory->tryInterpretConstant(a_, a)) {
    if (a == IntegerConstantType(1)) {
      out = ctxt.toWrap();
      return true;

    } else if (a == IntegerConstantType(-1)) {
      out = number::mul(a_, ctxt.toWrap());
      return true;

    } else {
      return false;
    }
  } else {
    return false;
  }
}

TermList doInvertMulInt(const InversionContext &ctxt) {
  CALL("doInvertMulInt(...)")
  TermList out;
  ALWAYS(tryInvertMulInt(ctxt, out)) 
  return out;
}

bool canInvertMulInt(const InversionContext &ctxt) {
  CALL("canInvertMulInt(const InversionContext&)")
  TermList _inv;
  return tryInvertMulInt(ctxt, _inv);
}

template <class number> bool nonZero(const TermList &t) {
  typename number::ConstantType c;
  return theory->tryInterpretConstant(t, c) && number::zeroC != c;
}


} // namespace Inverters
} // namespace Rebalancing
} // namespace Kernel
