//
// Created by jonas on 09.03.21.
//

#ifndef INFINIBAND_IBVEXCEPTION_HPP
#define INFINIBAND_IBVEXCEPTION_HPP


#include <exception>
#include <string>

class IBvException : public std::exception {
    public:
        explicit IBvException(int error);

        [[nodiscard]] const char *what() const noexcept override;

    private:
        std::string message;
};

inline void throwIfError(int error) {
    if (error != 0) {
        throw IBvException(error);
    }
}

inline void throwIfErrorErrno(int error){
    if (error != 0) {
        throw IBvException(errno);
    }
}

#endif //INFINIBAND_IBVEXCEPTION_HPP
