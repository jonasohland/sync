/// (c) Jonas Ohland


#include "c74_min.h"

namespace ohlano {


    using namespace c74::min;
    using namespace std::placeholders;

    class sync : public object<sync> {
    public:
        MIN_DESCRIPTION{"Synchronize Messages"};
        MIN_TAGS{"time"};
        MIN_AUTHOR{"Jonas Ohland"};
        MIN_RELATED{"pack, pak, metro, select, sel"};

        typedef struct atom_pipe {

            typedef enum {
                INT, FLOAT, BANG, LIST, ANY
            } msg_type;

            msg_type type;

            bool is_hot;

            bool isHot() {
                return is_hot;
            }

            std::unique_ptr<inlet<>> inlet_;
            std::unique_ptr<outlet<>> outlet_;

            atom_pipe() = delete;

            explicit atom_pipe(sync *instance, msg_type t, bool _is_hot = false) {

                type = t;

                this->is_hot = _is_hot;

                std::cout << "constructed : is_hot: " << ((is_hot) ? "true" : "false") << std::endl;

                auto outlet_gen = [=](sync *_inst, std::string _name, std::string _type) -> unique_ptr<outlet<>> {

                    return std::make_unique<outlet<>>(_inst, _name, _type);

                };

                auto inlet_gen = [=](sync *_inst, std::string _name, std::string _type) -> unique_ptr<inlet<>> {

                    return std::make_unique<inlet<>>(_inst, _name, _type);

                };

                std::string desc = (_is_hot) ? "hot " : "cold ";


                switch (t) {
                    case INT:
                        desc.append("integer inlet");
                        outlet_ = outlet_gen(instance, "integer outlet", "int");
                        inlet_ = inlet_gen(instance, desc, "int");

                        break;

                    case FLOAT:
                        desc.append("float inlet");

                        outlet_ = outlet_gen(instance, "float outlet", "float");
                        inlet_ = inlet_gen(instance, desc, "float");
                        break;

                    case BANG:
                        desc.append("bang inlet");

                        outlet_ = outlet_gen(instance, "bang outlet", "bang");
                        inlet_ = inlet_gen(instance, desc, "bang");
                        break;

                    case LIST:
                        desc.append("list inlet");

                        outlet_ = outlet_gen(instance, "list outlet", "list");
                        inlet_ = inlet_gen(instance, desc, "list");
                        break;

                    case ANY:
                        desc.append("symbols inlet");

                        outlet_ = std::make_unique<outlet<>>(instance, "symbols outlet");
                        inlet_ = std::make_unique<inlet<>>(instance, desc);
                        break;
                }
            }

            atoms storage_;
            bool was_banged = false;


            void push(const atoms &input) {
                if (type == BANG && !isHot()) {
                    was_banged = true;
                } else {
                    storage_.clear();
                    std::copy(input.begin(), input.end(), std::back_inserter(storage_));
                }
            }

            void let_out() {
                if (type == BANG) {
                    if (isHot()) {
                        outlet_->send("bang");
                    } else {
                        if (was_banged) {
                            was_banged = false;
                            outlet_->send("bang");
                        }
                    }

                } else {
                    outlet_->send(storage_);
                }
            }

            void clear() {
                if (type == BANG) {
                    was_banged = false;
                } else {
                    storage_.clear();
                }
            }

        } atom_pipe_t;

        std::vector<atom_pipe_t> pipes;

        bool mute = false;


        explicit sync(const atoms &args = {}) {

            if (!args.empty()) {

                for (int i = 0; i < args.size(); i++) {

                    std::string arg = args[i];

                    char type_char;
                    bool arg_is_hot = false;

                    if (arg[0] == 33) {
                        arg_is_hot = true;
                        type_char = arg[1];

                    } else {
                        type_char = arg[0];
                    }

                    switch (type_char) {
                        // i || I
                        case 0x69 :
                            pipes.emplace_back(this, atom_pipe_t::INT, arg_is_hot);
                            break;
                        case 0x49 :
                            pipes.emplace_back(this, atom_pipe_t::INT, arg_is_hot);
                            break;

                            //f || F
                        case 0x66 :
                            pipes.emplace_back(this, atom_pipe_t::FLOAT, arg_is_hot);
                            break;
                        case 0x46 :
                            pipes.emplace_back(this, atom_pipe_t::FLOAT, arg_is_hot);
                            break;

                            //b || B
                        case 0x62 :
                            pipes.emplace_back(this, atom_pipe_t::BANG, arg_is_hot);
                            break;
                        case 0x42 :
                            pipes.emplace_back(this, atom_pipe_t::BANG, arg_is_hot);
                            break;

                            //l || L
                        case 0x6C :
                            pipes.emplace_back(this, atom_pipe_t::LIST, arg_is_hot);
                            break;
                        case 0x4C :
                            pipes.emplace_back(this, atom_pipe_t::LIST, arg_is_hot);
                            break;

                            //s || S
                        case 0x73 :
                            pipes.emplace_back(this, atom_pipe_t::ANY, arg_is_hot);
                            break;
                        case 0x53 :
                            pipes.emplace_back(this, atom_pipe_t::ANY, arg_is_hot);
                            break;

                        default:
                            cerr << "no, try again (args)" << endl;
                    }
                }
            }
        }

        void outlet_all() {
            if (!mute) {
                std::for_each(pipes.rbegin(), pipes.rend(), [](auto &pipe) {
                    pipe.let_out();
                });
            }
        }

        atoms fire_all(const atoms &args, int inlet) {
            std::for_each(pipes.rbegin(), pipes.rend(), [](auto &pipe) {
                pipe.let_out();
            });
            return {};
        }

        void pipe_in(const atoms &args, int i, atom_pipe_t::msg_type t) {
            if (pipes[i].type == t) {
                pipes[i].push(args);
            }
            if (pipes[i].isHot()) {
                outlet_all();
            }
        }


        atoms handle_bang(const atoms &args, int inlet) {
            pipe_in(args, inlet, atom_pipe_t::BANG);
            return {};
        }

        atoms handle_float(const atoms &args, int inlet) {
            pipe_in(args, inlet, atom_pipe_t::FLOAT);
            return {};
        }

        atoms handle_int(const atoms &args, int inlet) {
            pipe_in(args, inlet, atom_pipe_t::INT);
            return {};
        }

        atoms handle_list(const atoms &args, int inlet) {
            pipe_in(args, inlet, atom_pipe_t::LIST);
            return {};
        }

        atoms handle_any(const atoms &args, int inlet) {
            pipe_in(args, inlet, atom_pipe_t::ANY);
            return {};
        }

        atoms handle_mute(const atoms &args, int inlet) {
            mute = true;
            return {};
        }

        atoms handle_unmute(const atoms &args, int inlet) {
            mute = false;
            return {};
        }

        atoms handle_clear(const atoms &args, int inlet) {
            for (auto &pipe : pipes) {
                pipe.clear();
            }
            return {};
        }


        message<> bang_msg{this, "bang", "handle bang message", std::bind(&sync::handle_bang, this, _1, _2)};
        message<> float_msg{this, "float", "handle float message", std::bind(&sync::handle_float, this, _1, _2)};
        message<> int_msg{this, "int", "handle int message", std::bind(&sync::handle_int, this, _1, _2)};
        message<> list_msg{this, "list", "handle list message", std::bind(&sync::handle_list, this, _1, _2)};
        message<> any_msg{this, "anything", "handle list message", std::bind(&sync::handle_any, this, _1, _2)};

        message<> mute_msg{this, "mute", "mute the output", std::bind(&sync::handle_mute, this, _1, _2)};
        message<> unmute_msg{this, "unmute", "unmute the output", std::bind(&sync::handle_unmute, this, _1, _2)};

        message<> clear_msg{this, "clear", "clear one or all pipes", std::bind(&sync::handle_clear, this, _1, _2)};

        message<> fire_msg{this, "fire", "fire all outlets", std::bind(&sync::fire_all, this, _1, _2)};


    };

}
MIN_EXTERNAL(ohlano::sync);


