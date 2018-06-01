#pragma once

#include <X11/Xlib.h>
#include "main.hh"

namespace wm {
    namespace matrix {
        enum HV {
            HORIZONTAL = true, VERTICAL = false
        };

        enum FB {
            FORWARD = true, BACKWARD = false
        };

        class Space {
            Display *const _display;
            const unsigned long _normal_pixel, _focus_pixel;
            const Atom _xia_protocols, _xia_delete_window;

            unsigned int _display_width, _display_height;
            HV _display_hv;

            Node *_root, *_view;
            node::Leaf *_focus;
            std::unordered_map<Window, node::Leaf *> _leaves;

            unsigned long _colorPixel(const char *const &);

            class _PointerCoordinates {
                const Space &_host;

                int _x, _y;

                Window _root, _child;
                int _root_x, _root_y, _win_x, _win_y;
                unsigned int _mask;

                void _refresh();

            public:
                explicit _PointerCoordinates(const Space &);

                void record();

                bool check();
            };

            _PointerCoordinates _pointer_coordinates;

        public:
            const server::EventHandlers event_handlers;

            explicit Space(Display *const &);

            void refresh();

            void focus(Node *const &);

            node::Branch *join(Node *const &, Node *const &, const FB &);

            friend Node;
            friend node::Branch;
            friend node::Leaf;
        };

        class Node {
        protected:
            Space *const _space;

            node::Branch *_parent;

            std::list<Node *>::iterator _iter_parent;
            HV _hv;
            int _x, _y;
            unsigned int _width, _height;

        public:
            explicit Node(Space *const &);

            virtual ~Node() = 0;

            virtual void configure(const HV &, const int &, const int &, const unsigned int &, const unsigned int &);

            virtual void refresh()=0;

            virtual node::Leaf *getActiveLeaf()=0;

        protected:
            virtual node::Branch *_receive(Node *const &, const FB &)=0;

            friend Space;
            friend node::Leaf;
            friend node::Branch;
        };

        namespace node {
            class Leaf : public Node {
                const Window _window;
                const std::unordered_map<Window, Leaf *>::iterator _iter_leaves;
            public:
                Leaf(Space *const &, const Window &);

                ~Leaf() final;

                void refresh() final;

                node::Leaf *getActiveLeaf() final;

            protected:
                node::Branch *_receive(Node *const &, const FB &) final;

                friend Space;
            };

            class Branch : public Node {
                std::list<Node *> _children;
                std::list<Node *>::iterator _active_iter;
            public:
                Branch(Space *const &);

                void configure(const HV &, const int &, const int &, const unsigned int &, const unsigned int &) final;

                void refresh() final;

                node::Leaf *getActiveLeaf() final;

            protected:
                node::Branch *_receive(Node *const &, const FB &) final;

            public:

                void configureChildren();

                friend Space;
                friend Leaf;
            };
        }

        const long root_event_mask = SubstructureNotifyMask;
        const long leaf_event_mask = FocusChangeMask | EnterWindowMask;

        auto matrix = [&](Display *const &display, const auto &callback) {
            auto space = new Space(display);
            space->refresh();

            server::CommandHandlers command_handlers = {};
            callback(
                    command_handlers,
                    root_event_mask,
                    leaf_event_mask,
                    space->event_handlers,
                    [&](const auto &break_, const std::string &handling_event_command_name, const auto &handleEvent) {
                        command_handlers["exit"] = break_;
                        command_handlers[handling_event_command_name] = handleEvent;
                    }
            );
        };
    }
}