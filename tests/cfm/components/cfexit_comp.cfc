component {
	this.name = "BEFORE_EXIT";
	writeOutput("CONSTRUCT_START|");
	exit;
	writeOutput("CONSTRUCT_AFTER");
	this.name = "AFTER_EXIT";
}
