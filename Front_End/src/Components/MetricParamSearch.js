import React, {Component} from 'react';
import './MetricParamSearch.css';

class MetricParamSearch extends Component{
    constructor(props){
        super(props);
        var array = [];
        props.metrics.map((obj,i) =>
            array.push({
                name: obj,
                val: i
            })
        )
        this.state = {
            selected: [],
            notSelected: array,
            optionsState: 0
        }
        this.removeSelected = this.removeSelected.bind(this)
    }

    change(event){
        console.log(event);
        this.setState({
            optionsState: event.target.value
        })
        event.preventDefault();
    }

    removeSelected(obj,i) {
        var arraySel = this.state.selected;
        var arrayNotSel = this.state.notSelected;
        arraySel.splice(i,1);
        arrayNotSel.push(obj);
        this.setState({
            selected: arraySel,
            notSelected: arrayNotSel
        })
        this.props.changeMetrics(arraySel)
    }

    addSelected(){
        var arraySel = this.state.selected;
        var arrayNotSel = this.state.notSelected;
        var obj = arrayNotSel[this.state.optionsState]
        console.log(obj)

        arrayNotSel.splice(this.state.optionsState, 1);
        arraySel.push(obj);
        console.log(arraySel)
        console.log(arrayNotSel)
        this.setState({
            selected: arraySel,
            notSelected: arrayNotSel
        })
        this.props.changeMetrics(arraySel)
    }

    render(){
        return(
            <div className="MetricParam">
                    {
                        this.state.selected.map((obj,i) =>
                            <div className="selectedMetric" key={i}>
                                {obj.name}
                                <a onClick={()=> this.removeSelected(obj,i)}>
                                    x
                                </a>
                            </div>   
                    )}
                    <div className="selectM">
                        <select value={this.state.optionsState} onChange={this.change.bind(this)}>
                            {
                                this.state.notSelected.map((obj,i) =>
                                    <option value={i} key={i} 
                                    className={(i===this.state.optionsState)?"selected":""}>
                                        {obj.name}
                                    </option>
                                )
                            }
                        </select>
                        
                        <a onClick={()=> this.addSelected()}>
                            +
                        </a>
                    </div>
            </div>
        )
    }
}

export default MetricParamSearch;