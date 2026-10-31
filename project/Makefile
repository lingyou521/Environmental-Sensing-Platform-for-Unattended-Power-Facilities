.PHONY: all app drivers clean

all: app drivers

app:
	$(MAKE) -C app

drivers:
	$(MAKE) -C drivers/sht30_drv
	$(MAKE) -C drivers/mq2_drv
	$(MAKE) -C drivers/door_drv
	$(MAKE) -C drivers/pir_drv
	$(MAKE) -C drivers/beep_drv

clean:
	$(MAKE) -C app clean
	$(MAKE) -C drivers/sht30_drv clean
	$(MAKE) -C drivers/beep_drv clean
	$(MAKE) -C drivers/mq2_drv clean
	$(MAKE) -C drivers/door_drv clean
	$(MAKE) -C drivers/pir_drv clean
