//
// Created by Anonymous on 2024/4/5.
//

#pragma once

#include <borealis.hpp>
#include <filesystem>

using namespace brls::literals;

const std::vector<std::string> extensions = {
    ".8svx", ".aac",  ".ac3",  ".aif",
    ".asf",  ".avi",  ".dv",   ".flv",
    ".m2ts", ".m2v",  ".m4a",  ".mkv",
    ".mov",  ".mp3",  ".mp4",  ".mpeg",
    ".mpg",  ".mts",  ".ogg",  ".rmvb",
    ".swf",  ".ts",   ".vob",  ".wav",
    ".wma",  ".wmv",  ".flac", ".m3u",
    ".m3u8", ".webm", ".jpg",  ".gif",
    ".png",  ".iso",
};

class Item {
public:
    bool is_up;
    std::filesystem::path path;
    bool is_directory;
    uintmax_t file_size;
};

const std::string recyclerCellXML = R"xml(
<brls:Box
    axis="row"
    focusable="true"
    alignItems="center"
    padding="10"
    cornerRadius="@style/brls/highlight/corner_radius"
    highlightCornerRadius="@style/brls/highlight/corner_radius">

    <brls:Image
        id="fileicon"
        width="44px"
        height="44px"
        marginLeft="@style/brls/sidebar/item_accent_margin_sides"
        marginRight="@style/brls/sidebar/item_accent_margin_sides" />

    <brls:Label
        id="filepath"
        width="auto"
        height="auto"
        grow="1.0"
        fontSize="@style/brls/sidebar/item_font_size"
        marginTop="@style/brls/sidebar/item_accent_margin_top_bottom"
        marginBottom="@style/brls/sidebar/item_accent_margin_top_bottom"
        marginLeft="@style/brls/sidebar/item_accent_margin_sides"
        marginRight="@style/brls/sidebar/item_accent_margin_sides" />
</brls:Box>
)xml";

class RecyclerCell : public brls::RecyclerCell
{
public:
    RecyclerCell();

    void setItem(std::string icon, Item *item);

    static RecyclerCell* create();
private:
    BRLS_BIND(brls::Label, filepath, "filepath");
    BRLS_BIND(brls::Image, fileicon, "fileicon");
};

class DirectoryDataSource : public brls::RecyclerDataSource
{
public:
    DirectoryDataSource(const std::filesystem::path& path = std::filesystem::current_path());

    void reloadData(const std::filesystem::path& path);

    int numberOfSections(brls::RecyclerFrame* recycler) override;
    int numberOfRows(brls::RecyclerFrame* recycler, int section) override;
    brls::RecyclerCell* cellForRow(brls::RecyclerFrame* recycler, brls::IndexPath index) override;
    void didSelectRowAt(brls::RecyclerFrame* recycler, brls::IndexPath indexPath) override;
    Item *getItem(int index);
private:
    std::vector<Item> items;
};

class LocalDriver : public brls::Box {
public:
    LocalDriver();

    static brls::View *create();
private:
    BRLS_BIND(brls::RecyclerFrame, recycler, "recycler");
};
