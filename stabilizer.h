template<class T> class stabilizer {
	float tolerance;
	T stabilized;
	bool first;

	public:
	stabilizer(float tolerance = 0.01) : tolerance(tolerance), stabilized(), first(true) {}
	T get() {return stabilized;}

	void operator=(T b) {
		if (first || isNan(stabilized)) {
			first = false;
			stabilized = b;
		}

		if (abs(stabilized - b) > tolerance) {
			T unit_vector = (stabilized - b)/abs(stabilized - b);

			stabilized = b + unit_vector*tolerance;
		}	
		//else do nothing
	};

	//implicitally change into type T
	operator T() const { return stabilized; };
};
// vim: ts=2 sw=2 filetype=cpp
