struct Point {
    int x;
    int y;
};

int point_sum(struct Point p) {
    return p.x + p.y;
}

int point_weighted_sum(struct Point p) {
    return (p.x * 10) + p.y;
}
