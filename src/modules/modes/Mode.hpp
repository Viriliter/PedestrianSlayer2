#ifndef MODE_HPP
#define MODE_HPP

namespace modes{

    class Mode{
    public:
        int modeID = 0;

        Mode(){};

        void begin(){}

        virtual void do_job() = 0; 

        void end(){}
    };
}
#endif  //MODE_HPP