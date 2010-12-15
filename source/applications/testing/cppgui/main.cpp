/* C++ gui tester application
 *
 * Copyright (c) 2010 Zoltan Kovacs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <iostream>

#include <ygui++/application.hpp>
#include <ygui++/window.hpp>
#include <ygui++/label.hpp>
#include <ygui++/panel.hpp>
#include <ygui++/button.hpp>
#include <ygui++/image.hpp>
#include <ygui++/imageloader.hpp>
#include <ygui++/menu.hpp>
#include <ygui++/menuitem.hpp>
#include <ygui++/scrollpanel.hpp>
#include <ygui++/textarea.hpp>
#include <ygui++/tabpanel.hpp>
#include <ygui++/layout/borderlayout.hpp>
#include <ygui++/text/plaindocument.hpp>

#include <yutil++/buffer/gapbuffer.hpp>

using namespace yguipp;
using namespace yguipp::layout;

class MyButton : public Button {
  public:
    MyButton(void) : Button("My button") {}
    Point getPreferredSize(void) { return Point(350, 350); }
};

void dumpElementTree(yguipp::text::Document* doc) {
    yguipp::text::Element* root = doc->getRootElement();
    std::cout << "elementCount=" << root->getElementCount() << std::endl;

    for (size_t i = 0; i < root->getElementCount(); i++) {
        yguipp::text::Element* e = root->getElement(i);
        //std::cout << "offset=" << e->getOffset() << " length=" << e->getLength() << " (" << doc->getText(e->getOffset(), e->getLength()) << ")" << std::endl;
        std::cout << "offset=" << e->getOffset() << " length=" << e->getLength() << std::endl;
    }
}

int main(int argc, char** argv) {
    Application::createInstance("cppguitest");
    ImageLoaderManager::getInstance()->loadLibraries();

    //Bitmap::loadFromFile("/system/images/file.png");

    Window* win = new Window( "Test", Point(50,50), Point(300,300) );
    win->init();

    Panel* container = dynamic_cast<Panel*>(win->getContainer());
    container->setLayout( new BorderLayout() );
    container->setBackgroundColor( Color(255,255,255) );

    container->add(new Label("PAGE_START"), new BorderLayoutData(BorderLayoutData::PAGE_START));
    MenuBar* menuBar = new MenuBar();
    //container->add(menuBar, new BorderLayoutData(BorderLayoutData::PAGE_START));

    MenuItem* file = new MenuItem("File");
    menuBar->add(file);
    MenuItem* help = new MenuItem("Help");
    menuBar->add(help);

    Menu* fileMenu = new Menu();
    file->setSubMenu(fileMenu);
    fileMenu->add(new MenuItem("Open"));
    MenuItem* save = new MenuItem("Save");
    fileMenu->add(save);
    fileMenu->add(new MenuItem("Exit"));

    Menu* helpMenu = new Menu();
    help->setSubMenu(helpMenu);
    helpMenu->add(new MenuItem("About"));

    Menu* saveMenu = new Menu();
    save->setSubMenu(saveMenu);
    saveMenu->add(new MenuItem("As Image"));
    saveMenu->add(new MenuItem("As Text"));

    container->add(new Label("PAGE_END"), new BorderLayoutData(BorderLayoutData::PAGE_END));
    container->add(new Label("LS"), new BorderLayoutData(BorderLayoutData::LINE_START));
    container->add(new Label("LE"), new BorderLayoutData(BorderLayoutData::LINE_END));

    //container->add(new Button("CENTER"), new BorderLayoutData(BorderLayoutData::CENTER));
    /*container->add(
        new Image(Bitmap::loadFromFile("/system/images/yaosp.png")),
        new BorderLayoutData(BorderLayoutData::CENTER)
        );*/
    //ScrollPanel* scrollPanel = new ScrollPanel();
    //scrollPanel->add(new MyButton());
    //container->add(scrollPanel, new BorderLayoutData(BorderLayoutData::CENTER));
    TextArea* textArea = new TextArea();
    textArea->getDocument()->insert(0, "Hello\nWorld!\nThis is the third line with a few characters\n...\n");
    //dumpElementTree(textArea->getDocument());

    Font* font = new yguipp::Font("DejaVu Sans Mono", "Book", 11);
    font->init();
    textArea->setFont(font);

    ScrollPanel* scrollPanel = new ScrollPanel();
    scrollPanel->add(textArea);

    //container->add(scrollPanel, new BorderLayoutData(BorderLayoutData::CENTER));

    TabPanel* tp = new TabPanel();
    tp->addTab("main.c", scrollPanel);
    tp->addTab("Hello World1", new Label("abc"));
    tp->addTab(":)", new Button("123"));

    container->add(tp, new BorderLayoutData(BorderLayoutData::CENTER));

    win->show();

    Application::getInstance()->mainLoop();

    return 0;
}

int main2(int argc, char** argv) {
    yguipp::text::PlainDocument* doc = new yguipp::text::PlainDocument();
    doc->insert(0, "Hello\nWorld");
    dumpElementTree(doc);
    std::cout << "======================================================" << std::endl;
    doc->insert(3, "LLL");
    dumpElementTree(doc);
    std::cout << "======================================================" << std::endl;
    doc->insert(3, "AAA\n");
    dumpElementTree(doc);
    std::cout << "======================================================" << std::endl;

    return 0;
}
