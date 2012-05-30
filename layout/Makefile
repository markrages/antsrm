BOARD_NAME:=antsrm
SCHMDIR=../schem

blah:
	echo

$(BOARD_NAME).net: $(SCHMDIR)/$(BOARD_NAME).sch
	gsch2pcb project-$(BOARD_NAME)

netlist: $(BOARD_NAME).net 

zip: $(BOARD_NAME).zip

$(BOARD_NAME).zip: $(BOARD_NAME).pcb
	mkdir -p gerbers
	cp $(BOARD_NAME).pcb gerbers
	cp $(BOARD_NAME).README gerbers || true

	cd gerbers && pcb -x gerber $(BOARD_NAME).pcb

	cd gerbers && combine_drill.py 	$(BOARD_NAME).plated-drill.cnc $(BOARD_NAME).unplated-drill.cnc > $(BOARD_NAME).drill.cnc


	cd gerbers && mv $(BOARD_NAME).front.gbr $(BOARD_NAME).gtl
	cd gerbers && mv $(BOARD_NAME).back.gbr $(BOARD_NAME).gbl
	cd gerbers && mv $(BOARD_NAME).frontmask.gbr $(BOARD_NAME).gts
	cd gerbers && mv $(BOARD_NAME).backmask.gbr $(BOARD_NAME).gbs
	cd gerbers && mv $(BOARD_NAME).frontsilk.gbr $(BOARD_NAME).gto
	cd gerbers && mv $(BOARD_NAME).backsilk.gbr $(BOARD_NAME).gbo
	cd gerbers && mv $(BOARD_NAME).drill.cnc $(BOARD_NAME).drl
	cd gerbers && mv $(BOARD_NAME).group5.gbr $(BOARD_NAME).outline


	cd gerbers && zip $(BOARD_NAME).zip \
	$(BOARD_NAME).gtl \
	$(BOARD_NAME).gbl \
	$(BOARD_NAME).gts \
	$(BOARD_NAME).gbs \
	$(BOARD_NAME).gto \
	$(BOARD_NAME).gbo \
	$(BOARD_NAME).drl \
	$(BOARD_NAME).outline

	mv gerbers/$(BOARD_NAME).zip .


$(BOARD_NAME).ps: $(BOARD_NAME).pcb
	pcb -x ps $(BOARD_NAME).pcb

$(BOARD_NAME).pdf: $(BOARD_NAME).ps
	ps2pdf $(BOARD_NAME).ps $(BOARD_NAME).pdf

pdf: $(BOARD_NAME).pdf 
	ln -sf $(BOARD_NAME).pdf layout.pdf

clean:
	rm -f gnetlist.log text.ps text.pcb \
	$(BOARD_NAME)-front.png $(BOARD_NAME)-back.png $(BOARD_NAME)-render.png
	rm -rf gerbers
	rm -rf $(BOARD_NAME).ps $(BOARD_NAME).pdf layout.pdf

clean-net: clean
	rm -f $(BOARD_NAME).net $(BOARD_NAME).cmd fixed*

clean-pcb: clean
	rm -f text-top.pcb text-bottom.pcb

text-top.pcb: text-top.svg
	inkscape text-top.svg -p '> /tmp/text-top.ps'
	#pstoedit -flat 0.02 -ssp -dt -f pcbfill text.ps text.pcb 
	pstoedit -flat 0.02 -ssp -f pcbfill /tmp/text-top.ps text-top.pcb 
	rm -f /tmp/text-top.ps

text-bottom.pcb: text-bottom.svg
	inkscape text-bottom.svg -p '> /tmp/text-bottom.ps'
	#pstoedit -flat 0.02 -ssp -dt -f pcbfill text.ps text.pcb 
	pstoedit -flat 0.02 -ssp -f pcbfill /tmp/text-bottom.ps text-bottom.pcb 
	rm -f /tmp/text-bottom.ps

%.png: %.pcb
	pcbrender $< $@ 

render: $(BOARD_NAME).png
	true

publish: $(BOARD_NAME).png
	scp $^ kaonashi:/var/www/midwest-telecine-live
	#scp -P2022 $^ vivara.net:/var/www/midwest-telecine-live
	touch publish

$(BOARD_NAME).bom: $(SCHMDIR)/$(BOARD_NAME).sch
	echo "value" > attribs
	gnetlist  -g bom2 -o $(BOARD_NAME).bom $(SCHMDIR)/$(BOARD_NAME).sch 
	# preserve header
	( head -1 $(BOARD_NAME).bom ; tail -n +2 $(BOARD_NAME).bom | sort ) | sed s/:/\\\t/g > $(BOARD_NAME).bom.sorted
	mv -f $(BOARD_NAME).bom.sorted $(BOARD_NAME).bom

bom: $(BOARD_NAME).bom
