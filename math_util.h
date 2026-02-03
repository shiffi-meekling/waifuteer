#pragma once

///rescales value when it is in [bottom, top] to [0, 1] 
#include <concepts>
template<class T> T rescale(T value, T bottom, T top) 
requires (!std::integral<T>)
{
	return (value - bottom)	/ (top - bottom);
}

///Not really thrilled by this functions handling of types
template<class T, class S> T linear_interpolate(T start, T end, S steps, S step) 
requires (std::integral<S>)
{
	double step_size = static_cast<double>(end - start) / steps;
	return start + static_cast<T>(step*step_size);

}
// vim: ts=2 sw=2
