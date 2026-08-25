component {
    this.year = "unset";
    this.month = "unset";
    this.day = "unset";

    function init(year, month, day) {
        setYear(arguments.year);
        setMonth(arguments.month);
        setDay(arguments.day);
        return this;
    }

    function setYear(value) {
        this.year = "Y" & value;
    }

    function setMonth(value) {
        this.month = "M" & value;
    }

    function setDay(value) {
        this.day = "D" & value;
    }
}
