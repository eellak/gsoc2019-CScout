import React, {Component} from 'react';
import './MetricParamSearch.css';
import X from './x.ico';

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
                    <b> Metrics</b>
                    {
                        this.state.selected.map((obj,i) =>
                            <div className="selectedMetric" key={i}>
                                <div>
                                    {obj.name}
                                </div>
                                <span className="helper"></span>
                                <a onClick={()=> this.removeSelected(obj,i)} style={{cursor:"pointer"}}>
                                    <img src={X} height={10} width={10} alt="x"/>
                                </a>
                                
                            </div>   
                    )}
                    <div className="selectM">
                        {(this.state.notSelected.length > 0)?
                        <div>
                            <select value={this.state.optionsState} onChange={this.change.bind(this)} 
                                className="selectEl" size="5">
                                {
                                    this.state.notSelected.map((obj,i) =>
                                        <option value={i} key={i} 
                                        className={((i + 3) === this.state.optionsState)?"selected":""}>
                                            {obj.name}
                                        </option>
                                    )
                                }
                            </select>
                            <a onClick={()=> this.addSelected()} style={{cursor: "pointer"}}>
                            +
                            </a>
                         </div>:
                        null
                        }
                        
                        
                    </div>
            </div>
        )
    }
}

export default MetricParamSearch;