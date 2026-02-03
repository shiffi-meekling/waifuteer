#pragma once
#include <algorithm>
#include <cmath>
#include <concepts>
#include <exception>
#include <format>
#include <memory>
#include <span>
#include <vector>
#include <ranges>

template<class A, class B> float l1_distance(const A& a, const B& b) {
	using std::ranges::views::zip_transform;
	using std::ranges::fold_left;
	auto distances = zip_transform([](auto i, auto j){return std::abs(i - j);}, a, b);
	return std::ranges::fold_left(distances, 0, std::plus{});
}

/**
this maps a n-dimentional vector to nearest T value
*/
template<class T>
class ndim_knn_map {
	//the float keys, all concatenated together 
	std::vector<float> backing;
	std::vector<T> mapped_to;
	size_t m_dimentions = 1;
	public:
	using mapped_type = T;
	using key_type = std::vector<float>;
	using value_type = std::pair<key_type, T>;
	
	//lifetime management	
	ndim_knn_map() = default;
	///create an empty map of given dimentions
	explicit ndim_knn_map(size_t dim) : m_dimentions{dim} {}
	///create from a 
	ndim_knn_map(std::initializer_list<value_type> pairs) 
		: m_dimentions{pairs.begin()->first.size()} 
	{
		reserve(pairs.size());
		for (auto&& [k,v] : pairs) {
			push_back(k,v);
		}
	}
	ndim_knn_map(ndim_knn_map<T>&& a)  = default; 
	ndim_knn_map(const ndim_knn_map<T>&) = default; 
	ndim_knn_map<T>& operator=(const ndim_knn_map<T>&&) = default;
	ndim_knn_map<T>& operator=(const ndim_knn_map<T>&) = default;
	~ndim_knn_map() = default;

	size_t dimentions() const {return m_dimentions;}
	size_t size() const {return mapped_to.size();}
	void reserve(size_t amount) {
		backing.reserve(amount * m_dimentions);
		mapped_to.reserve(amount);
	}
	
	const T& get(const key_type& x) const {
		//contract_assert(backing.size() % m_dimentions == 0)
		if (x.size() != m_dimentions) {
			throw std::invalid_argument(
				std::format("passed vector's length {} must be equal to this->dimentions() {}",
					 x.size(), m_dimentions));
		}
		//contract_assert(m_dimentions * mapped_to.size() == backing.size())
	
		float min_distance = HUGE_VALF;
		size_t min_index = 0;
		for (auto i = backing.begin(); i != backing.end(); i += m_dimentions) {
			std::span<const float>  point(i, m_dimentions);
			float distance = l1_distance(point, x);
			if ( distance < min_distance) {
				min_distance = distance;
				min_index = i - backing.begin();
			}  
		}
		//now that we know what key is the closest
		//we can lookup its actual result	
		return mapped_to.at(min_index / m_dimentions);
	}

	void push_back(const key_type& key, const mapped_type&& value) {
		//first thing we put in sets the dimention of the key
		if (mapped_to.empty()) {
			m_dimentions = key.size();
		} else if (key.size() != m_dimentions) {
			throw std::invalid_argument(
				std::format("passed vector's length {} must be equal to this->dimentions() {}",
					 key.size(), m_dimentions));
		}
		std::copy(key.begin(), key.end(), std::back_insert_iterator(backing));
		mapped_to.emplace_back(value);
	}

	void push_back(const key_type& key, const mapped_type& value) {
		if (key.size() != m_dimentions) {
			throw std::invalid_argument(
				std::format("passed vector's length {} must be equal to this->dimentions() {}",
					 key.size(), m_dimentions));
		}
		std::copy(key.begin(), key.end(), std::back_insert_iterator(backing));
		mapped_to.emplace_back(value);
	}
	
	private:
	struct iterator {
		const ndim_knn_map<T>* host;
		size_t index = 0;
		void operator++() {
			++index;
		}

		bool operator==(iterator &rhs) {
			return index >= rhs.index;
		}

		value_type operator*() const {
			const key_type& key = 
					{host->backing.begin() + index*host->m_dimentions,
					 host->backing.begin() + (index+1) * host->m_dimentions};
			const mapped_type& value = host->mapped_to.at(index);
			return {key, value};
		}

		std::unique_ptr<value_type> operator->() const {
			return std::make_unique<value_type>(this->operator*());
		}
	};

	public:
	iterator begin() const {
		return iterator{this, 0};
	}

	iterator end() const {
		return {this, mapped_to.size()};
	}
};
// vim: ts=2 sw=2
