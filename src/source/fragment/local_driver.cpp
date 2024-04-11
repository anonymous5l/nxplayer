//
// Created by Anonymous on 2024/4/5.
//

#include "utils/intent.hpp"

#include "fragment/local_driver.hpp"

DirectoryDataSource::DirectoryDataSource(const std::filesystem::path& path) {
    this->reloadData(path);
}

void DirectoryDataSource::reloadData(const std::filesystem::path& path) {
    this->items.clear();

    auto iter = std::filesystem::directory_iterator(path);

    for (const auto& fp : iter) {
        if (!fp.exists() || fp.path().filename().string()[0] == '.') {
            continue;
        }

        auto is_directory = fp.is_directory();

        if (std::find(extensions.begin(), extensions.end(),
                      fp.path().extension().string()) != extensions.end() || is_directory) {
            auto item = Item {
                    .path = fp.path(),
                    .is_directory = is_directory,
                    .file_size = 0,
            };

            if (!item.is_directory) {
                item.file_size = fp.file_size();
            }

            this->items.push_back(item);
        }
    }

    std::sort(this->items.begin(), this->items.end(), [](const Item& a, const Item& b) {
        return a.path.filename().string() < b.path.filename().string();
    });

#ifdef __SWITCH__
    if (path.parent_path().string() != "sdmc:")
#else
    if (path.has_parent_path() && path.parent_path() != path.root_path())
#endif
    {
        this->items.insert(this->items.begin(), Item{
                .is_up = true,
                .path = path.parent_path(),
                .is_directory = true,
                .file_size = 0,
        });
    }
#ifdef __SWITCH__
    else if (path.string() != "sdmc:/") {
        this->items.insert(this->items.begin(), Item{
            .is_up = true,
            .path = "sdmc:/",
            .is_directory = true,
            .file_size = 0,
        });
    }
#endif
}

int DirectoryDataSource::numberOfSections(brls::RecyclerFrame* recycler) {
    return 1;
}

int DirectoryDataSource::numberOfRows(brls::RecyclerFrame* recycler, int section) {
    return (int)this->items.size();
}

brls::RecyclerCell* DirectoryDataSource::cellForRow(brls::RecyclerFrame* recycler, brls::IndexPath index) {
    RecyclerCell* item = (RecyclerCell*)recycler->dequeueReusableCell("Cell");
    auto f = this->items[index.row];
    if (f.is_directory) {
        item->setItem("folder", &f);
    } else {
        item->setItem("file", &f);
    }
    return item;
}

Item *DirectoryDataSource::getItem(int index) {
    if (!this->items.empty()) {
        return &this->items[index];
    }
    return nullptr;
}

void DirectoryDataSource::didSelectRowAt(brls::RecyclerFrame* recycler, brls::IndexPath index) {
    auto f = this->items[index.row];
    if (f.is_directory) {
        this->reloadData(f.path);
        recycler->reloadData();
    } else {
        // open video view
        Intent::openVideo(f.path.filename().string(), f.path.string());
    }
}

RecyclerCell::RecyclerCell() {
    this->inflateFromXMLString(recyclerCellXML);
}

void RecyclerCell::setItem(std::string icon, Item *item) {
    if (brls::Application::getThemeVariant() == brls::ThemeVariant::DARK) {
        this->fileicon->setImageFromRes(fmt::format("svg/ico-dark-{}.svg", icon));
    } else {
        this->fileicon->setImageFromRes(fmt::format("svg/ico-light-{}.svg", icon));
    }

    if (item->is_up) {
        this->filepath->setText("..");
    } else {
        this->filepath->setText(item->path.filename().string());
    }
}

RecyclerCell *RecyclerCell::create() {
    return new RecyclerCell();
}

LocalDriver::LocalDriver() {
    this->inflateFromXMLRes("xml/fragment/local_driver.xml");

    this->recycler->estimatedRowHeight = 70;
    this->recycler->registerCell("Cell", []() { return RecyclerCell::create(); });
    DirectoryDataSource *dataSource = nullptr;
#ifdef __SWITCH__
    dataSource = new DirectoryDataSource("sdmc:/");
#else
    dataSource = new DirectoryDataSource();
#endif
    this->recycler->setDataSource(dataSource);

    this->recycler->registerAction("hints/back"_i18n, brls::BUTTON_B, [this, dataSource](...) {
        auto item = dataSource->getItem(0);
        brls::Logger::info("Local Driver back trigger.");
        if (item != nullptr && item->is_up) {
            dataSource->didSelectRowAt(this->recycler, brls::IndexPath(0, 0));
            return true;
        }
        return false;
    }, true);
}

brls::View* LocalDriver::create() {
    return (brls::View*)(new LocalDriver());
}