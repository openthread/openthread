#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "spi_interface.hpp"
#include "lib/url/url.hpp"

class MockUrl : public ot::Url::Url
{
public:
    // TODO: add MOCK_METHOD
};

TEST(SpiInterfaceTest, Constructor_Success)
{
    MockUrl                 mockUrl;
    ot::Posix::SpiInterface spiInterface(mockUrl);
}
