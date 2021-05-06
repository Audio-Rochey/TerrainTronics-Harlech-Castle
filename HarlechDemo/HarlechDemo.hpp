#ifndef _HARLECH_DEMO_HPP_
#define _HARLECH_DEMO_HPP_

#include "Harlech.hpp"
#include "HarlechPinConfig.hpp"
#include "KeepAlive.hpp"
#include "Patterns.hpp"


struct PatternStep {
  int ledPattern;
  int brightness;
  int duration;
};

class HarlechCastleDemoPattern {
public:
  std::string name;
  std::vector<PatternStep> steps;

  // Set Up and initialize the pointers used for patterns                          
  size_t currentIndex = 0;

  template <size_t N>
  HarlechCastleDemoPattern( const char *name, int (&pattern)[N][3] ) : name(name) {
    for( size_t x = 0; x < N; x++ ) {
      PatternStep step { pattern[x][0], pattern[x][1], pattern[x][2] };
      steps.push_back( step );
    }
  }

  const PatternStep &currentStep() const { return steps[currentIndex]; }
  void advance() { 
    if( ++currentIndex >= steps.size() ) {
      currentIndex  = 0; }
    }
  
};
typedef std::shared_ptr<HarlechCastleDemoPattern> PatternPtr;
class HarlechCastleDemo {
public: 
  
  HarlechCastleDemo() :
      nextAdvance(0)
      {}

  const HarlechCastleDemoPattern &pattern() const { return *activePattern; }
  HarlechCastleDemoPattern &pattern() { return *activePattern; }

  template <size_t N>
  HarlechCastleDemo &addPattern( const char *name, int (&pattern)[N][3] ) {
    HarlechCastleDemoPattern p( name, pattern );
    auto pShared = std::make_shared<HarlechCastleDemoPattern>(p);
    _patterns.push_back(pShared);
    patternMap[pShared->name] = pShared;
    if( !activePattern ) {
      activePattern = pShared;
    }
    return *this;
  }
  size_t patterns() const { return _patterns.size(); }
  
  void setPattern( size_t index ) {
    index = std::min( index, _patterns.size()-1 );
    activePattern = _patterns[index];
  }

  void loop();
  void setup();

protected:
  size_t nextAdvance;
  std::vector<PatternPtr> _patterns;
  std::map<std::string, PatternPtr> patternMap;
  PatternPtr activePattern;

};


#endif // _HARLECH_DEMO_HPP_
