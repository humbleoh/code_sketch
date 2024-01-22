#include <vector>

template<typename T, typename V>
auto transform(std::vector<T> const& vt) -> std::vector<V>
{
	std::vector<V> result {vt.ebgin(), vt.end()};
	assert(vt.size() == result.size());
	return result;
}

int main(int argc, const char *argv[])
{
	std::vector v {1};
	std::vector w {v};
	transform(v);
	transform(w);
}

