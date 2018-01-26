#pragma once

struct Camera
{
  claws::Vect<2u, double> offset{50.0, 50.0};
  claws::Vect<2u, float> zoom{0.5f, 0.37f};

  template<class T>
  constexpr claws::Vect<2u, float> apply(claws::Vect<2u, T> pos) const
  {
    return claws::Vect<2u, float>(pos + offset) * zoom;
  }

  template<class T>
  constexpr claws::Vect<2u, double> unapply(claws::Vect<2u, T> pos) const
  {
    return  static_cast<claws::Vect<2u, double>>(pos / zoom) - offset;
  }
};
