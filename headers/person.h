#ifndef PERSON_H
#define PERSON_H

#include "object.h"

class Person : public Object {
  public:
    Person() {}

    void Init();
    void Update();
    void Draw();

  private:

    sf::Sprite mySpr;

};

#endif // PERSON_H
