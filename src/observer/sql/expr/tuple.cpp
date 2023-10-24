#include"tuple.h"
Tuple * copy_tuple(Tuple* src){
  switch (src->tuple_type())
  {
  case ROW:
    return new RowTuple(*static_cast<RowTuple *>(src));
    break;
  case JOIN:
    return new JoinedTuple(*static_cast<JoinedTuple *>(src));
    break;
  case PROJECT:
    return new ProjectTuple(*static_cast<ProjectTuple *>(src));
    break;
  case VALUE:
    return new ValueListTuple(*static_cast<ValueListTuple *>(src));
    break;
  default:
    return new RowTuple(*static_cast<RowTuple *>(src));
    break;
  }
}