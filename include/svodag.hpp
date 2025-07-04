#ifndef SVODAG_H
#define SVODAG_H

class SvoDag {
public:
	SvoDag(int level);

private:
	int level;

	void dedup();
}

#endif
