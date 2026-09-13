#include "flowmini_testabi.h"

int point_sum(FlowminiTestAbiPoint p) {
    return p.x + p.y;
}

int point_weighted_sum(FlowminiTestAbiPoint p) {
    return (p.x * 10) + p.y;
}
