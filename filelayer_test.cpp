#include "filelayer.h"
#include "./libs/doctest.h"

TEST_CASE("can find local files?") {
	CHECK_NOTHROW(filelayer::canonise_path("./kantan-chan/body.PNG"));
	CHECK_THROWS(filelayer::canonise_path("./kantan-chan/does_not_exist_at_all.PNG"));
}


TEST_CASE("find user files" * doctest::skip()) {
///this test should normally be skipped because it only works on shiffi's computer
	CHECK_NOTHROW(filelayer::canonise_path("./shiffi/neck.png"));

}

// vim: ts=2 sw=2
