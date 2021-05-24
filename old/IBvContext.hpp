/**
 * @file IBvContext.hpp.h
 * @author ottojo
 * @date 2/24/21
 * Description here TODO
 */

#ifndef INFINIBAND_IBVCONTEXT_HPP
#define INFINIBAND_IBVCONTEXT_HPP


class IBvContext {
    public:
        explicit IBvContext(struct ibv_device *);

        explicit IBvContext(struct ibv_context *c);

        IBvContext(const IBvContext &) = delete;

        IBvContext &operator=(const IBvContext &) = delete;

        ~IBvContext();

        struct ibv_context *get();

        struct ibv_device_attr queryAttributes();

        struct ibv_port_attr queryPort(int port);

    private:
        struct ibv_context *context;

};


#endif //INFINIBAND_IBVCONTEXT_HPP
