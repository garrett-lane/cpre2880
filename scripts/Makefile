run2:
	python3 app.py

run: build
	./a.out | python3 compare_smoothed_data.py -d

build:
	gcc test.c
