class MoveValidation
{
public:
    MoveValidation(bool valid,
                   MoveValidationReason reason);

    bool isValid;
    MoveValidationReason reason;
};