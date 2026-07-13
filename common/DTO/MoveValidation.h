#ifndef MOVEVALIDATION_H
#define MOVEVALIDATION_H

#include "../enums/MoveValidationReason.h"

class MoveValidation
{
public:
    MoveValidation(bool valid,
                   MoveValidationReason reason)
        : isValid(valid),
          reason(reason)
    {
    }

    bool isValid;
    MoveValidationReason reason;
};

#endif
