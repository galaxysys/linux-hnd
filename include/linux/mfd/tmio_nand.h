struct tmio_nand_hwconfig {
        void (*hwinit)(struct platform_device *sdev);
        void (*suspend)(struct platform_device *sdev);
        void (*resume)(struct platform_device *sdev);
};

