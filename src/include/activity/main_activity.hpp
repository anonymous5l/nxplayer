//
// Created by Anonymous on 2024/4/3.
//

#pragma once

#include <borealis.hpp>

class MainActivity : public brls::Activity {
public:
    // Declare that the content of this activity is the given XML file
    CONTENT_FROM_XML_RES("activity/main.xml");
};
