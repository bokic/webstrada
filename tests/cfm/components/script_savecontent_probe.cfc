component {
    public function render() {
        savecontent variable="local.data" {
            writeOutput("included-content");
        }
        return local.data;
    }
}
