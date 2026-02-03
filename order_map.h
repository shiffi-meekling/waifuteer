#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <initializer_list>

///map that also records order
template<class K, class V> class order_map {
	//assert map.size() == order.size()
	std::unordered_map<K, V> map;
	std::vector<K> order;

	public:
	using key_type = K;
	using mapped_type = V;
	using value_type = std::pair<const K&, const V&>;
	using value_type_no_ref = std::pair<const K, const V>;

	order_map() = default;
	order_map(const order_map<K,V>& a) = default;
	order_map(order_map<K,V>&& a) = default;
	order_map& operator=(order_map&& a) = default; 
	order_map& operator=(const order_map& a) = default; 

	order_map(std::initializer_list<value_type_no_ref> a) { 
		for (auto [key, value] : a) {
			insert(key, value);
		}
	}

	void insert(const K& key, const V& value) {
		order.push_back(key);
		map[key] = value;
	}
	
	bool contains(const K& key) const {
		return map.contains(key);
	}

	V& at(const K& key) {
		return map.at(key);
	}

	const V& at(const K& key) const {
		return map.at(key);
	}

	V& operator[](const K& key) {
		if (!map.contains(key)) {
			order.push_back(key);
		} 
		return map[key];
	}

	const V& operator[](const K& key) const {
		return map.at(key);
	}

	size_t size() const {
		return order.size();
	}

	class iterator {
		public:
		size_t index = 0;
		const order_map* host;
		void operator++() {
			++index;
		}

		bool operator==(iterator &rhs) {
			return index >= rhs.index;
		}

		value_type operator*() const {
			const K& key = host->order[index];
			return {key, host->map.at(key)};
		}

		std::unique_ptr<value_type> operator->() const {
			return std::make_unique<value_type>(this->operator*());
		}
	};

	iterator begin() const {
		iterator foo;
		foo.index = 0;
		foo.host = this;
		return foo;
	}	

	iterator end() const {
		iterator foo;
		foo.host = this;
		foo.index = foo.host->size();
		return foo;
	}
};


static_assert(requires {order_map<int, float>{}.begin()->first;});
static_assert(requires {order_map<int, float>{{1,2.f}};});


// vim: ts=2 sw=2
