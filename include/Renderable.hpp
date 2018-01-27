#ifndef RENDERABLE_HPP
# define RENDERABLE_HPP

# include "my_opengl.hpp"
# include "claws/Vect.hpp"

struct Renderable
{
  // TEXTURE
  /// bottom left corner of source rect
  claws::Vect<2u, float> sourcePos{0.0f, 0.0f};
  /// size of source rect
  claws::Vect<2u, float> sourceSize{0.0f, 0.0f};

  // DISPLAY
  /// center of dest position
  claws::Vect<2u, float> destPos{0.0f, 0.0f};
  /// size when displayed
  claws::Vect<2u, float> destSize{0.0f, 0.0f};
};

#endif
