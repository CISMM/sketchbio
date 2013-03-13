#ifndef GROUPIDGENERATOR_H
#define GROUPIDGENERATOR_H

class GroupIdGenerator {
public:
    virtual ~GroupIdGenerator() {}
    virtual int getNextGroupId() = 0;
};

#endif // GROUPIDGENERATOR_H
