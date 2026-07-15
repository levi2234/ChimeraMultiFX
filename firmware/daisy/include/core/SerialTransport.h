#pragma once

#include "daisy_seed.h"

class SerialTransport {
public:
    static constexpr size_t RX_QUEUE_LEN = 512;
    static constexpr uint32_t UART_BAUD = 115200;
    static constexpr uint32_t UART_TX_MAX_TIMEOUT_MS = 2000;
    using ByteCallback = void (*)(char byte, void* context);

    void Init(daisy::UsbHandle* usb, daisy::UartHandler* uart) {
        usb_ = usb;
        uart_ = uart;
    }

    void StartUartDmaReceive(
        uint8_t* buffer,
        size_t size,
        daisy::UartHandler::CircularRxCallbackFunctionPtr callback,
        void* context) {
        uart_rx_dma_buffer_ = buffer;
        uart_rx_dma_buffer_size_ = size;
        uart_rx_dma_callback_ = callback;
        uart_rx_dma_context_ = context;
        if (uart_) {
            rx_started_ = uart_->DmaListenStart(buffer, size, callback, context)
                == daisy::UartHandler::Result::OK;
            if (!rx_started_) rx_restart_failure_count_++;
        }
    }

    bool MaintainUartReceive() {
        if (!uart_ || !uart_rx_dma_buffer_ || !uart_rx_dma_callback_) return false;
        if (rx_started_ && uart_->IsListening() && uart_->CheckError() == 0) return false;

        uart_->DmaListenStop();
        rx_started_ = uart_->DmaListenStart(
                uart_rx_dma_buffer_, uart_rx_dma_buffer_size_,
                uart_rx_dma_callback_, uart_rx_dma_context_)
            == daisy::UartHandler::Result::OK;
        if (rx_started_) {
            rx_restart_count_++;
        } else {
            rx_restart_failure_count_++;
        }
        return true;
    }

    void QueueUartBytes(const uint8_t* data, size_t size) {
        rx_byte_count_ += size;
        for (size_t i = 0; i < size; i++) {
            const uint16_t next = static_cast<uint16_t>((rx_head_ + 1) % RX_QUEUE_LEN);
            if (next == rx_tail_) {
                rx_overflowed_ = true;
                continue;
            }
            rx_queue_[rx_head_] = data[i];
            rx_head_ = next;
        }
    }

    void ProcessPending(ByteCallback callback, void* context) {
        if (!callback) return;
        char byte = 0;
        while (Dequeue(byte)) callback(byte, context);
    }

    bool TakeRxOverflow() {
        const bool overflowed = rx_overflowed_;
        if (overflowed) rx_tail_ = rx_head_;
        rx_overflowed_ = false;
        return overflowed;
    }

    uint32_t TxFailureCount() const {
        return tx_failure_count_;
    }

    uint32_t RxRestartCount() const {
        return rx_restart_count_;
    }

    uint32_t RxRestartFailureCount() const {
        return rx_restart_failure_count_;
    }

    uint32_t RxByteCount() const {
        return rx_byte_count_;
    }

    void Send(const char* data, size_t size) {
        if (size == 0) return;
        if (usb_) {
            usb_->TransmitInternal(reinterpret_cast<uint8_t*>(const_cast<char*>(data)), size);
        }
        if (!uart_) return;

        uint32_t timeout_ms = static_cast<uint32_t>(
            (size * 10000u + UART_BAUD - 1u) / UART_BAUD + 50u);
        if (timeout_ms > UART_TX_MAX_TIMEOUT_MS) timeout_ms = UART_TX_MAX_TIMEOUT_MS;
        if (uart_->BlockingTransmit(
            reinterpret_cast<uint8_t*>(const_cast<char*>(data)), size, timeout_ms)
            != daisy::UartHandler::Result::OK) {
            tx_failure_count_++;
        }
    }

private:
    bool Dequeue(char& byte) {
        if (rx_tail_ == rx_head_) return false;
        byte = static_cast<char>(rx_queue_[rx_tail_]);
        rx_tail_ = static_cast<uint16_t>((rx_tail_ + 1) % RX_QUEUE_LEN);
        return true;
    }

    daisy::UsbHandle* usb_ = nullptr;
    daisy::UartHandler* uart_ = nullptr;
    uint8_t rx_queue_[RX_QUEUE_LEN] = {};
    volatile uint16_t rx_head_ = 0;
    volatile uint16_t rx_tail_ = 0;
    volatile bool rx_overflowed_ = false;
    volatile uint32_t tx_failure_count_ = 0;
    volatile uint32_t rx_restart_count_ = 0;
    volatile uint32_t rx_restart_failure_count_ = 0;
    volatile uint32_t rx_byte_count_ = 0;
    bool rx_started_ = false;
    uint8_t* uart_rx_dma_buffer_ = nullptr;
    size_t uart_rx_dma_buffer_size_ = 0;
    daisy::UartHandler::CircularRxCallbackFunctionPtr uart_rx_dma_callback_ = nullptr;
    void* uart_rx_dma_context_ = nullptr;
};